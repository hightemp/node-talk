#include "PeerManager.h"

#include "PeerLink.h"
#include "TransportServer.h"
#include "app/Logger.h"
#include "core/EventLog.h"
#include "core/Identity.h"
#include "core/Settings.h"
#include "db/Database.h"

#include <QDateTime>
#include <QJsonObject>
#include <QTcpSocket>
#include <QUuid>

namespace nodetalk::net {

PeerManager::PeerManager(Database& db, Identity& identity, Settings& settings,
                         EventLog& events, TransportServer& transport,
                         DiscoveryService& discovery, QObject* parent)
    : QObject(parent),
      m_db(db), m_identity(identity), m_settings(settings),
      m_events(events), m_transport(transport), m_discovery(discovery),
      m_peerRepo(db), m_msgRepo(db)
{
    connect(&m_transport, &TransportServer::incomingSocket,
            this, &PeerManager::onIncomingSocket);
    connect(&m_discovery, &DiscoveryService::peerAnnounced,
            this, &PeerManager::onPeerAnnounced);
    connect(&m_discovery, &DiscoveryService::peerByeReceived,
            this, &PeerManager::onPeerByeReceived);

    m_sweep.setInterval(2000);
    connect(&m_sweep, &QTimer::timeout, this, &PeerManager::onSweep);
    m_sweep.start();

    loadPeers();
}

PeerManager::~PeerManager()
{
    for (auto* link : m_links.values()) {
        link->close();
        link->deleteLater();
    }
    for (auto* link : m_pendingLinks) {
        link->close();
        link->deleteLater();
    }
}

void PeerManager::loadPeers()
{
    const auto rows = m_peerRepo.all();
    for (const auto& p : rows) m_peers.insert(p.id, p);
}

QList<Peer> PeerManager::allPeers() const
{
    QList<Peer> out;
    out.reserve(m_peers.size());
    for (auto it = m_peers.constBegin(); it != m_peers.constEnd(); ++it) {
        Peer p = it.value();
        p.online = m_links.contains(p.id);
        out.push_back(p);
    }
    std::sort(out.begin(), out.end(), [](const Peer& a, const Peer& b) {
        if (a.online != b.online) return a.online > b.online;
        return a.shownName().compare(b.shownName(), Qt::CaseInsensitive) < 0;
    });
    return out;
}

std::optional<Peer> PeerManager::findPeer(const QString& id) const
{
    auto it = m_peers.constFind(id);
    if (it == m_peers.constEnd()) return std::nullopt;
    Peer p = it.value();
    p.online = m_links.contains(p.id);
    return p;
}

bool PeerManager::isOnline(const QString& peerId) const
{
    return m_links.contains(peerId);
}

PeerLink* PeerManager::linkFor(const QString& peerId) const
{
    return m_links.value(peerId, nullptr);
}

QString PeerManager::sendText(const QString& peerId, const QString& body)
{
    auto* link = linkFor(peerId);
    if (!link || link->state() != PeerLink::State::Ready) return {};
    auto p = findPeer(peerId);
    if (!p || p->trust != TrustState::Trusted) return {};

    Message m;
    m.id        = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m.peerId    = peerId;
    m.direction = MessageDirection::Outgoing;
    m.body      = body;
    m.ts        = QDateTime::currentSecsSinceEpoch();
    m.status    = MessageStatus::Sending;
    m.kind      = MessageKind::Text;
    m_msgRepo.insert(m);
    emit messageAppended(m);

    QJsonObject text{
        {QStringLiteral("type"),   QString::fromLatin1(msg::kText)},
        {QStringLiteral("msg_id"), m.id},
        {QStringLiteral("ts"),     m.ts},
        {QStringLiteral("body"),   body},
    };
    link->sendJson(text);

    m_msgRepo.updateStatus(m.id, MessageStatus::Sent);
    emit messageStatusChanged(m.id, MessageStatus::Sent);
    return m.id;
}

void PeerManager::sendTyping(const QString& peerId, bool active)
{
    auto* link = linkFor(peerId);
    if (!link || link->state() != PeerLink::State::Ready) return;
    QJsonObject t{
        {QStringLiteral("type"),   QString::fromLatin1(msg::kTyping)},
        {QStringLiteral("active"), active},
    };
    link->sendJson(t);
}

void PeerManager::markRead(const QString& peerId, const QString& messageId)
{
    auto* link = linkFor(peerId);
    if (!link || link->state() != PeerLink::State::Ready) return;
    QJsonObject ack{
        {QStringLiteral("type"),   QString::fromLatin1(msg::kTextAck)},
        {QStringLiteral("msg_id"), messageId},
        {QStringLiteral("status"), QStringLiteral("read")},
    };
    link->sendJson(ack);
}

void PeerManager::trust(const QString& peerId)
{
    if (!m_peers.contains(peerId)) return;
    Peer& p = m_peers[peerId];
    p.trust = TrustState::Trusted;
    m_peerRepo.setTrust(peerId, TrustState::Trusted);
    m_events.info(QStringLiteral("trust_accepted"),
                  QStringLiteral("Trusted peer %1").arg(p.shownName()), peerId);
    emit peerUpdated(p);
}

void PeerManager::block(const QString& peerId)
{
    if (!m_peers.contains(peerId)) return;
    Peer& p = m_peers[peerId];
    p.trust = TrustState::Blocked;
    m_peerRepo.setTrust(peerId, TrustState::Blocked);
    if (auto* link = m_links.value(peerId)) link->close();
    emit peerUpdated(p);
}

void PeerManager::rename(const QString& peerId, const QString& newDisplayName)
{
    if (!m_peers.contains(peerId)) return;
    Peer& p = m_peers[peerId];
    p.displayName = newDisplayName;
    m_peerRepo.setDisplayName(peerId, newDisplayName);
    emit peerUpdated(p);
}

void PeerManager::connectTo(const QHostAddress& addr, quint16 port)
{
    auto* link = new PeerLink(m_identity.peerId(), m_identity.fingerprint(),
                              m_settings.displayName(), this);
    attachLinkSignals(link);
    m_pendingLinks.append(link);
    link->connectToHost(addr, port);
}

void PeerManager::onIncomingSocket(QTcpSocket* sock)
{
    auto* link = new PeerLink(sock, PeerLink::Direction::Incoming,
                              m_identity.peerId(), m_identity.fingerprint(),
                              m_settings.displayName(), this);
    attachLinkSignals(link);
    m_pendingLinks.append(link);
}

void PeerManager::attachLinkSignals(PeerLink* link)
{
    connect(link, &PeerLink::ready,         this, [this, link] { onLinkReady(link); });
    connect(link, &PeerLink::disconnected,  this, [this, link](const QString& r) { onLinkDisconnected(link, r); });
    connect(link, &PeerLink::jsonReceived,  this, [this, link](const QJsonObject& o){ onLinkJson(link, o); });
    connect(link, &PeerLink::binaryReceived,this, [this, link](const QByteArray& b){ onLinkBinary(link, b); });
}

Peer& PeerManager::upsertPeerFromAnnounce(const PeerAnnouncement& a)
{
    auto it = m_peers.find(a.peerId);
    if (it == m_peers.end()) {
        Peer p;
        p.id          = a.peerId;
        p.fingerprint = a.fingerprint;
        p.nickname    = a.nickname;
        p.lastIp      = a.address.toString();
        p.lastPort    = a.tcpPort;
        p.lastSeen    = a.ts;
        p.createdAt   = QDateTime::currentSecsSinceEpoch();
        p.trust       = TrustState::Unknown;
        it = m_peers.insert(a.peerId, p);
        m_peerRepo.upsert(p);
        m_events.info(QStringLiteral("peer_discovered"),
                      QStringLiteral("Discovered %1 (%2)").arg(p.shownName(), p.lastIp), p.id);
        emit peerAdded(p);
    } else {
        it->nickname = a.nickname.isEmpty() ? it->nickname : a.nickname;
        it->lastIp   = a.address.toString();
        it->lastPort = a.tcpPort;
        it->lastSeen = a.ts;
        m_peerRepo.touch(it->id, it->lastIp, it->lastPort, it->lastSeen);
        emit peerUpdated(*it);
    }
    return *it;
}

void PeerManager::onPeerAnnounced(const PeerAnnouncement& a)
{
    Peer& p = upsertPeerFromAnnounce(a);
    if (p.trust == TrustState::Blocked) return;
    if (!m_links.contains(p.id)) {
        ensureConnection(p);
    }
}

void PeerManager::onPeerByeReceived(const QString& peerId)
{
    if (auto* link = m_links.value(peerId)) link->close();
}

void PeerManager::ensureConnection(const Peer& p)
{
    if (p.lastIp.isEmpty() || p.lastPort == 0) return;

    // Avoid simultaneous-open races: only the side with the lexicographically
    // smaller peer_id initiates the connection.
    if (m_identity.peerId() >= p.id) return;

    auto* link = new PeerLink(m_identity.peerId(), m_identity.fingerprint(),
                              m_settings.displayName(), this);
    attachLinkSignals(link);
    m_pendingLinks.append(link);
    link->connectToHost(QHostAddress(p.lastIp), p.lastPort);
}

void PeerManager::onLinkReady(PeerLink* link)
{
    const QString rid = link->remotePeerId();
    if (rid.isEmpty() || rid == m_identity.peerId()) {
        link->close();
        return;
    }

    auto it = m_peers.find(rid);
    if (it == m_peers.end()) {
        Peer p;
        p.id          = rid;
        p.fingerprint = link->remoteFingerprint();
        p.nickname    = link->remoteNickname();
        p.lastIp      = link->remoteAddress().toString();
        p.lastPort    = link->remotePort();
        p.lastSeen    = QDateTime::currentSecsSinceEpoch();
        p.createdAt   = p.lastSeen;
        p.trust       = TrustState::Unknown;
        it = m_peers.insert(rid, p);
        m_peerRepo.upsert(p);
        emit peerAdded(p);
    } else {
        if (it->fingerprint != link->remoteFingerprint() && it->trust == TrustState::Trusted) {
            // Different fingerprint for a trusted peer id: demote to unknown.
            it->trust = TrustState::Unknown;
            m_peerRepo.setTrust(it->id, TrustState::Unknown);
        }
        it->fingerprint = link->remoteFingerprint();
        it->nickname    = link->remoteNickname();
        it->lastIp      = link->remoteAddress().toString();
        it->lastPort    = link->remotePort();
        it->lastSeen    = QDateTime::currentSecsSinceEpoch();
        m_peerRepo.upsert(*it);
        emit peerUpdated(*it);
    }

    if (it->trust == TrustState::Blocked) {
        link->close();
        return;
    }

    m_pendingLinks.removeAll(link);
    if (auto* prev = m_links.value(rid)) {
        if (prev != link) {
            prev->close();
            m_linkToPeer.remove(prev);
            prev->deleteLater();
        }
    }
    m_links.insert(rid, link);
    m_linkToPeer.insert(link, rid);
    setOnline(rid, true);

    if (it->trust == TrustState::Unknown) {
        emit trustPromptRequested(*it);
    }
}

void PeerManager::onLinkDisconnected(PeerLink* link, const QString& reason)
{
    Q_UNUSED(reason);
    const QString rid = m_linkToPeer.value(link);
    m_linkToPeer.remove(link);
    if (!rid.isEmpty()) {
        m_links.remove(rid);
        setOnline(rid, false);
    }
    m_pendingLinks.removeAll(link);
    link->deleteLater();
}

void PeerManager::setOnline(const QString& peerId, bool online)
{
    if (auto it = m_peers.find(peerId); it != m_peers.end()) {
        it->online = online;
        emit peerOnlineChanged(peerId, online);
        emit peerUpdated(*it);
    }
}

void PeerManager::onLinkJson(PeerLink* link, const QJsonObject& obj)
{
    const QString rid  = m_linkToPeer.value(link);
    if (rid.isEmpty()) return;
    const QString type = obj.value(QStringLiteral("type")).toString();

    auto p = findPeer(rid);
    const bool trusted = p && p->trust == TrustState::Trusted;

    if (type == QString::fromLatin1(msg::kText)) {
        if (!trusted) return;
        Message m;
        m.id        = obj.value(QStringLiteral("msg_id")).toString();
        m.peerId    = rid;
        m.direction = MessageDirection::Incoming;
        m.body      = obj.value(QStringLiteral("body")).toString();
        m.ts        = static_cast<qint64>(obj.value(QStringLiteral("ts")).toDouble());
        m.status    = MessageStatus::Delivered;
        m.kind      = MessageKind::Text;
        m_msgRepo.insert(m);
        emit messageAppended(m);
        // Auto-deliver-ack
        QJsonObject ack{
            {QStringLiteral("type"),   QString::fromLatin1(msg::kTextAck)},
            {QStringLiteral("msg_id"), m.id},
            {QStringLiteral("status"), QStringLiteral("delivered")},
        };
        link->sendJson(ack);
        return;
    }

    if (type == QString::fromLatin1(msg::kTextAck)) {
        const QString id = obj.value(QStringLiteral("msg_id")).toString();
        const QString st = obj.value(QStringLiteral("status")).toString();
        const auto s = (st == QStringLiteral("read")) ? MessageStatus::Read : MessageStatus::Delivered;
        m_msgRepo.updateStatus(id, s);
        emit messageStatusChanged(id, s);
        return;
    }

    if (type == QString::fromLatin1(msg::kTyping)) {
        if (!trusted) return;
        const bool active = obj.value(QStringLiteral("active")).toBool();
        emit typingChanged(rid, active);
        return;
    }

    // File transfer messages: only forward when trusted.
    if (trusted && (type == QString::fromLatin1(msg::kFileOffer)
                 || type == QString::fromLatin1(msg::kFileAccept)
                 || type == QString::fromLatin1(msg::kFileReject)
                 || type == QString::fromLatin1(msg::kFileChunkReq)
                 || type == QString::fromLatin1(msg::kFileChunkMeta)
                 || type == QString::fromLatin1(msg::kFilePause)
                 || type == QString::fromLatin1(msg::kFileResume)
                 || type == QString::fromLatin1(msg::kFileComplete))) {
        emit fileMessageReceived(rid, obj);
        return;
    }
}

void PeerManager::onLinkBinary(PeerLink* link, const QByteArray& body)
{
    const QString rid = m_linkToPeer.value(link);
    if (rid.isEmpty()) return;
    auto p = findPeer(rid);
    if (!p || p->trust != TrustState::Trusted) return;
    emit fileBinaryReceived(rid, body);
}

void PeerManager::onSweep()
{
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 timeout = m_settings.peerTimeoutSec();
    for (auto it = m_peers.begin(); it != m_peers.end(); ++it) {
        const bool linked = m_links.contains(it->id);
        const bool freshAnnounce = (now - it->lastSeen) <= timeout;
        const bool online = linked || freshAnnounce;
        if (online != it->online) {
            it->online = online;
            emit peerOnlineChanged(it->id, online);
            emit peerUpdated(*it);
            if (!online) {
                m_events.info(QStringLiteral("peer_lost"),
                              QStringLiteral("Peer %1 went offline").arg(it->shownName()), it->id);
            }
        }
    }
}

void PeerManager::persistPeer(const Peer& p)
{
    m_peerRepo.upsert(p);
}

} // namespace nodetalk::net
