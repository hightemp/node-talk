#pragma once

#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

class QUdpSocket;

namespace nodetalk {
class Identity;
class Settings;

namespace net {

/// Information carried by a discovery `announce` packet.
struct PeerAnnouncement {
    QString      peerId;
    QString      fingerprint;
    QString      nickname;
    QHostAddress address;
    quint16      tcpPort = 0;
    qint64       ts      = 0;
};

/// UDP discovery service. Sends periodic announces over both broadcast
/// and an administratively-scoped multicast group, and emits signals
/// whenever a remote peer is heard from.
class DiscoveryService : public QObject {
    Q_OBJECT
public:
    DiscoveryService(Identity& id, Settings& s, quint16 tcpPort, QObject* parent = nullptr);
    ~DiscoveryService() override;

    bool start();
    void stop();

    /// Forces an immediate announce.
    void announceNow();

    /// Update the TCP port advertised in announces (e.g. when transport
    /// is restarted with a different port).
    void setTcpPort(quint16 port);

signals:
    void peerAnnounced(const nodetalk::net::PeerAnnouncement& a);
    void peerByeReceived(const QString& peerId);

private slots:
    void onBroadcastReadyRead();
    void onMulticastReadyRead();
    void onTick();

private:
    void readDatagrams(QUdpSocket& s);
    void handleDatagram(const QByteArray& data, const QHostAddress& from);
    void send(const QByteArray& body);

    Identity& m_identity;
    Settings& m_settings;
    quint16   m_tcpPort = 0;

    std::unique_ptr<QUdpSocket> m_broadcast;
    std::unique_ptr<QUdpSocket> m_multicast;
    QTimer m_tick;
    bool   m_running = false;
};

} // namespace net
} // namespace nodetalk
