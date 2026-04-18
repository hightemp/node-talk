#pragma once

#include "DiscoveryService.h"
#include "db/PeerRepository.h"
#include "db/MessageRepository.h"
#include "model/Message.h"
#include "model/Peer.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>

class QTcpSocket;

namespace nodetalk {
class Database;
class Identity;
class Settings;
class EventLog;

namespace net {

class PeerLink;
class TransportServer;

/// The brain of the peer subsystem.
///
/// Reconciles discovery announcements + persistent peer records into a
/// unified live `Peer` view, owns every `PeerLink`, enforces the trust
/// model, persists messages and exposes a clean Qt-signals API to the
/// UI and the file-transfer manager.
class PeerManager : public QObject {
    Q_OBJECT
public:
    PeerManager(Database& db,
                Identity& identity,
                Settings& settings,
                EventLog& events,
                TransportServer& transport,
                DiscoveryService& discovery,
                QObject* parent = nullptr);
    ~PeerManager() override;

    // ------------- peers ---------------
    QList<Peer>            allPeers() const;
    std::optional<Peer>    findPeer(const QString& id) const;
    bool                   isOnline(const QString& peerId) const;

    // ------------- chat ----------------
    /// Sends a text message to `peerId`. Returns the message id, or an
    /// empty string if the peer is offline / untrusted (UI should
    /// disable its send button in that case).
    QString sendText(const QString& peerId, const QString& body);

    void sendTyping(const QString& peerId, bool active);
    void markRead(const QString& peerId, const QString& messageId);

    // ------------- trust ---------------
    void trust(const QString& peerId);
    void block(const QString& peerId);
    void rename(const QString& peerId, const QString& newDisplayName);

    // ------------- manual add ----------
    void connectTo(const QHostAddress& addr, quint16 port);

    // Internal: file transfer module needs the link to write frames.
    PeerLink* linkFor(const QString& peerId) const;

signals:
    void peerAdded(const Peer& p);
    void peerUpdated(const Peer& p);
    void peerOnlineChanged(const QString& peerId, bool online);

    void messageAppended(const Message& m);
    void messageStatusChanged(const QString& messageId, MessageStatus s);
    void typingChanged(const QString& peerId, bool active);

    /// File-transfer related JSON arrived from a remote peer — passed
    /// through to FileTransferManager.
    void fileMessageReceived(const QString& peerId, const QJsonObject& obj);
    void fileBinaryReceived(const QString& peerId, const QByteArray& body);

    /// Trust prompt request: a previously-unknown or fingerprint-changed
    /// peer is trying to connect.
    void trustPromptRequested(const Peer& p);

private slots:
    void onIncomingSocket(QTcpSocket* sock);
    void onPeerAnnounced(const PeerAnnouncement& a);
    void onPeerByeReceived(const QString& peerId);
    void onLinkReady(PeerLink* link);
    void onLinkDisconnected(PeerLink* link, const QString& reason);
    void onLinkJson(PeerLink* link, const QJsonObject& obj);
    void onLinkBinary(PeerLink* link, const QByteArray& body);
    void onSweep();

private:
    void   loadPeers();
    void   ensureConnection(const Peer& p);
    void   attachLinkSignals(PeerLink* link);
    void   bindLinkToPeer(PeerLink* link);
    Peer&  upsertPeerFromAnnounce(const PeerAnnouncement& a);
    void   persistPeer(const Peer& p);
    void   setOnline(const QString& peerId, bool online);

    Database&         m_db;
    Identity&         m_identity;
    Settings&         m_settings;
    EventLog&         m_events;
    TransportServer&  m_transport;
    DiscoveryService& m_discovery;

    PeerRepository    m_peerRepo;
    MessageRepository m_msgRepo;

    QHash<QString, Peer>      m_peers;        // peer_id -> Peer (cache)
    QHash<QString, PeerLink*> m_links;        // peer_id -> link
    QHash<PeerLink*, QString> m_linkToPeer;
    QList<PeerLink*>          m_pendingLinks; // pre-handshake
    QTimer                    m_sweep;
};

} // namespace net
} // namespace nodetalk
