#pragma once

#include "db/TransferRepository.h"
#include "model/Transfer.h"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

class QFile;

namespace nodetalk {
class Database;
class Settings;
class EventLog;

namespace net {

class PeerManager;

/// File transfer state machine.
///
/// The protocol is receiver-driven: the receiver asks for the next
/// chunk it wants by index, the sender produces it. This makes pause
/// (receiver stops asking), resume (receiver asks for the next
/// missing index) and reconnect-after-disconnect (receiver re-issues
/// `file_accept` with the bytes-on-disk offset) trivial.
///
/// Integrity is validated by SHA-256, computed lazily on the sender,
/// shipped in the offer, and verified on the receiver after the last
/// chunk lands.
class FileTransferManager : public QObject {
    Q_OBJECT
public:
    FileTransferManager(Database& db, Settings& settings,
                        EventLog& events, PeerManager& peers,
                        QObject* parent = nullptr);
    ~FileTransferManager() override;

    /// Queue a file for outgoing transfer to `peerId`. Returns the
    /// transfer id, or empty on validation failure (offline, untrusted,
    /// missing file).
    QString queueOutgoing(const QString& peerId, const QString& localPath);

    /// Accept an incoming offer; saves to `savePath` (if empty, uses the
    /// configured downloads dir).
    void acceptIncoming(const QString& xferId, const QString& savePath = {});
    void rejectIncoming(const QString& xferId, const QString& reason = {});

    void pause(const QString& xferId);
    void resume(const QString& xferId);
    void cancel(const QString& xferId);

    QList<Transfer> activeTransfers() const;
    QList<Transfer> recentTransfers(int limit = 200) const;

signals:
    void transferAdded(const Transfer& t);
    void transferUpdated(const Transfer& t);
    void incomingOffer(const Transfer& t);

private slots:
    void onPeerJson(const QString& peerId, const QJsonObject& obj);
    void onPeerBinary(const QString& peerId, const QByteArray& body);

private:
    struct Outgoing {
        Transfer t;
        QFile*   file = nullptr;
        int      chunkSize = 65536;
        int      totalChunks = 0;
        int      nextIndex = 0;     // hint
        QString  peerId;
    };
    struct Incoming {
        Transfer t;
        QFile*   file = nullptr;
        int      chunkSize = 65536;
        int      totalChunks = 0;
        int      nextIndex = 0;     // chunk waiting for binary body
        int      pendingChunkSize = 0;
        QString  peerId;
        bool     paused = false;
    };

    void   sendChunk(Outgoing& o, int chunkIndex);
    void   requestNext(Incoming& in);
    void   finishIncomingOk(Incoming& in);
    void   finishIncomingFail(Incoming& in, const QString& reason);
    void   eraseOutgoing(const QString& id);
    void   eraseIncoming(const QString& id);

    Database&    m_db;
    Settings&    m_settings;
    EventLog&    m_events;
    PeerManager& m_peers;

    TransferRepository m_repo;

    QHash<QString, Outgoing*> m_outgoing; // xfer_id -> state
    QHash<QString, Incoming*> m_incoming; // xfer_id -> state
};

} // namespace net
} // namespace nodetalk
