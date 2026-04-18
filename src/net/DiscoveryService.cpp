#include "DiscoveryService.h"

#include "Protocol.h"
#include "app/Logger.h"
#include "app/Version.h"
#include "core/Identity.h"
#include "core/Settings.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QUdpSocket>

namespace nodetalk::net {

DiscoveryService::DiscoveryService(Identity& id, Settings& s, quint16 tcpPort, QObject* parent)
    : QObject(parent), m_identity(id), m_settings(s), m_tcpPort(tcpPort)
{
    m_tick.setInterval(5000);
    connect(&m_tick, &QTimer::timeout, this, &DiscoveryService::onTick);
}

DiscoveryService::~DiscoveryService() = default;

bool DiscoveryService::start()
{
    if (m_running) return true;

    const quint16 dport = static_cast<quint16>(m_settings.discoveryPort());
    const quint16 mport = static_cast<quint16>(m_settings.multicastPort());
    const QHostAddress mgrp(m_settings.multicastGroup());

    m_broadcast = std::make_unique<QUdpSocket>(this);
    m_broadcast->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
    if (!m_broadcast->bind(QHostAddress::AnyIPv4, dport,
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCWarning(ntDisc) << "broadcast bind failed:" << m_broadcast->errorString();
    }
    connect(m_broadcast.get(), &QUdpSocket::readyRead,
            this, &DiscoveryService::onBroadcastReadyRead);

    m_multicast = std::make_unique<QUdpSocket>(this);
    m_multicast->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
    if (!m_multicast->bind(QHostAddress::AnyIPv4, mport,
                           QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qCWarning(ntDisc) << "multicast bind failed:" << m_multicast->errorString();
    }
    const auto ifaces = QNetworkInterface::allInterfaces();
    for (const auto& ifc : ifaces) {
        if (!(ifc.flags() & QNetworkInterface::IsUp)) continue;
        if (!(ifc.flags() & QNetworkInterface::CanMulticast)) continue;
        m_multicast->joinMulticastGroup(mgrp, ifc);
    }
    connect(m_multicast.get(), &QUdpSocket::readyRead,
            this, &DiscoveryService::onMulticastReadyRead);

    m_running = true;
    announceNow();
    m_tick.start();
    qCInfo(ntDisc) << "Discovery started, broadcast:" << dport
                   << "multicast:" << m_settings.multicastGroup() << mport;
    return true;
}

void DiscoveryService::stop()
{
    if (!m_running) return;
    m_tick.stop();

    QJsonObject bye{
        {QStringLiteral("type"),    QString::fromLatin1(msg::kAnnounce)},
        {QStringLiteral("subtype"), QStringLiteral("bye")},
        {QStringLiteral("peer_id"), m_identity.peerId()},
    };
    send(QJsonDocument(bye).toJson(QJsonDocument::Compact));

    if (m_broadcast) m_broadcast->close();
    if (m_multicast) m_multicast->close();
    m_broadcast.reset();
    m_multicast.reset();
    m_running = false;
}

void DiscoveryService::setTcpPort(quint16 port) { m_tcpPort = port; }

void DiscoveryService::announceNow()
{
    QJsonObject ann{
        {QStringLiteral("type"),         QString::fromLatin1(msg::kAnnounce)},
        {QStringLiteral("proto"),        kProtocolVersion},
        {QStringLiteral("peer_id"),      m_identity.peerId()},
        {QStringLiteral("fingerprint"),  m_identity.fingerprint()},
        {QStringLiteral("nick"),         m_settings.displayName()},
        {QStringLiteral("tcp_port"),     m_tcpPort},
        {QStringLiteral("ts"),           QDateTime::currentSecsSinceEpoch()},
        {QStringLiteral("app_version"),  QString::fromLatin1(kAppVersion)},
    };
    send(QJsonDocument(ann).toJson(QJsonDocument::Compact));
}

void DiscoveryService::onTick() { announceNow(); }

void DiscoveryService::onBroadcastReadyRead()
{
    if (m_broadcast) readDatagrams(*m_broadcast);
}
void DiscoveryService::onMulticastReadyRead()
{
    if (m_multicast) readDatagrams(*m_multicast);
}

void DiscoveryService::readDatagrams(QUdpSocket& s)
{
    while (s.hasPendingDatagrams()) {
        QNetworkDatagram dg = s.receiveDatagram();
        handleDatagram(dg.data(), dg.senderAddress());
    }
}

void DiscoveryService::handleDatagram(const QByteArray& data, const QHostAddress& from)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    const QJsonObject o = doc.object();
    if (o.value(QStringLiteral("type")).toString() != QString::fromLatin1(msg::kAnnounce)) return;

    const QString peerId = o.value(QStringLiteral("peer_id")).toString();
    if (peerId.isEmpty() || peerId == m_identity.peerId()) return;

    if (o.value(QStringLiteral("subtype")).toString() == QStringLiteral("bye")) {
        emit peerByeReceived(peerId);
        return;
    }

    PeerAnnouncement a;
    a.peerId      = peerId;
    a.fingerprint = o.value(QStringLiteral("fingerprint")).toString();
    a.nickname    = o.value(QStringLiteral("nick")).toString();
    a.tcpPort     = static_cast<quint16>(o.value(QStringLiteral("tcp_port")).toInt());
    a.ts          = static_cast<qint64>(o.value(QStringLiteral("ts")).toDouble(QDateTime::currentSecsSinceEpoch()));
    a.address     = from.isNull() ? QHostAddress(QHostAddress::LocalHost) : from;
    if (a.fingerprint.isEmpty() || a.tcpPort == 0) return;
    emit peerAnnounced(a);
}

void DiscoveryService::send(const QByteArray& body)
{
    if (!m_running && !m_broadcast && !m_multicast) return;
    const quint16 dport = static_cast<quint16>(m_settings.discoveryPort());
    const quint16 mport = static_cast<quint16>(m_settings.multicastPort());
    const QHostAddress mgrp(m_settings.multicastGroup());

    if (m_broadcast) {
        m_broadcast->writeDatagram(body, QHostAddress::Broadcast, dport);
    }
    if (m_multicast) {
        m_multicast->writeDatagram(body, mgrp, mport);
    }
}

} // namespace nodetalk::net
