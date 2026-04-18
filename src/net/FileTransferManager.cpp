#include "FileTransferManager.h"

#include "PeerLink.h"
#include "PeerManager.h"
#include "Protocol.h"
#include "app/Logger.h"
#include "core/EventLog.h"
#include "core/Settings.h"
#include "db/Database.h"
#include "db/MessageRepository.h"
#include "model/Message.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QUuid>

namespace nodetalk::net {

namespace {

QString sha256OfFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash h(QCryptographicHash::Sha256);
    if (!h.addData(&f)) return {};
    return QString::fromLatin1(h.result().toHex());
}

QString uniqueSavePath(const QString& dir, const QString& base)
{
    QDir d(dir);
    QFileInfo fi(base);
    QString stem = fi.completeBaseName();
    QString suf  = fi.suffix();
    QString candidate = d.filePath(base);
    int n = 1;
    while (QFile::exists(candidate)) {
        candidate = d.filePath(suf.isEmpty()
            ? QStringLiteral("%1 (%2)").arg(stem).arg(n)
            : QStringLiteral("%1 (%2).%3").arg(stem).arg(n).arg(suf));
        ++n;
    }
    return candidate;
}

} // namespace

FileTransferManager::FileTransferManager(Database& db, Settings& settings,
                                         EventLog& events, PeerManager& peers,
                                         QObject* parent)
    : QObject(parent),
      m_db(db), m_settings(settings), m_events(events), m_peers(peers),
      m_repo(db)
{
    connect(&m_peers, &PeerManager::fileMessageReceived,
            this,    &FileTransferManager::onPeerJson);
    connect(&m_peers, &PeerManager::fileBinaryReceived,
            this,    &FileTransferManager::onPeerBinary);
}

FileTransferManager::~FileTransferManager()
{
    for (auto* o : m_outgoing.values()) { if (o->file) o->file->close(); delete o; }
    for (auto* i : m_incoming.values()) { if (i->file) i->file->close(); delete i; }
}

QString FileTransferManager::queueOutgoing(const QString& peerId, const QString& localPath)
{
    auto* link = m_peers.linkFor(peerId);
    if (!link || link->state() != PeerLink::State::Ready) return {};
    QFileInfo fi(localPath);
    if (!fi.exists() || !fi.isFile()) return {};

    auto* o = new Outgoing;
    o->peerId         = peerId;
    o->chunkSize      = m_settings.chunkSize();
    o->t.id           = QUuid::createUuid().toString(QUuid::WithoutBraces);
    o->t.peerId       = peerId;
    o->t.direction    = TransferDirection::Outgoing;
    o->t.fileName     = fi.fileName();
    o->t.fileSize     = fi.size();
    o->t.localPath    = fi.absoluteFilePath();
    o->t.mime         = QMimeDatabase().mimeTypeForFile(fi).name();
    o->t.sha256       = sha256OfFile(o->t.localPath);
    o->t.bytesDone    = 0;
    o->t.state        = TransferState::Pending;
    o->t.startedAt    = QDateTime::currentSecsSinceEpoch();
    o->totalChunks    = static_cast<int>((o->t.fileSize + o->chunkSize - 1) / o->chunkSize);

    o->file = new QFile(o->t.localPath, this);
    if (!o->file->open(QIODevice::ReadOnly)) { delete o->file; delete o; return {}; }

    m_repo.upsert(o->t);
    m_outgoing.insert(o->t.id, o);

    // Announce as a chat item too.
    MessageRepository msgRepo(m_db);
    Message m;
    m.id        = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m.peerId    = peerId;
    m.direction = MessageDirection::Outgoing;
    m.body      = QStringLiteral("📎 %1 (%2 bytes)").arg(o->t.fileName).arg(o->t.fileSize);
    m.ts        = o->t.startedAt;
    m.status    = MessageStatus::Sending;
    m.kind      = MessageKind::File;
    msgRepo.insert(m);
    o->t.messageId = m.id;
    m_repo.upsert(o->t);

    QJsonObject offer{
        {QStringLiteral("type"),       QString::fromLatin1(msg::kFileOffer)},
        {QStringLiteral("xfer_id"),    o->t.id},
        {QStringLiteral("name"),       o->t.fileName},
        {QStringLiteral("size"),       static_cast<double>(o->t.fileSize)},
        {QStringLiteral("mime"),       o->t.mime},
        {QStringLiteral("sha256"),     o->t.sha256},
        {QStringLiteral("chunk_size"), o->chunkSize},
        {QStringLiteral("chunks"),     o->totalChunks},
    };
    link->sendJson(offer);
    m_events.info(QStringLiteral("file_offered"),
                  QStringLiteral("Offered %1 to %2").arg(o->t.fileName, peerId), peerId);

    emit transferAdded(o->t);
    return o->t.id;
}

void FileTransferManager::acceptIncoming(const QString& xferId, const QString& savePath)
{
    auto* in = m_incoming.value(xferId, nullptr);
    if (!in) return;
    auto* link = m_peers.linkFor(in->peerId);
    if (!link) return;

    QString dir = QFileInfo(savePath).absolutePath();
    QString file = QFileInfo(savePath).fileName();
    if (savePath.isEmpty()) {
        dir = m_settings.downloadsDir();
        file = in->t.fileName;
    }
    QDir().mkpath(dir);
    in->t.localPath = uniqueSavePath(dir, file);

    in->file = new QFile(in->t.localPath, this);
    if (!in->file->open(QIODevice::WriteOnly)) {
        finishIncomingFail(*in, QStringLiteral("cannot open output file"));
        return;
    }
    in->t.state     = TransferState::Active;
    in->t.startedAt = QDateTime::currentSecsSinceEpoch();
    in->totalChunks = static_cast<int>((in->t.fileSize + in->chunkSize - 1) / in->chunkSize);
    m_repo.upsert(in->t);
    emit transferUpdated(in->t);

    QJsonObject acc{
        {QStringLiteral("type"),         QString::fromLatin1(msg::kFileAccept)},
        {QStringLiteral("xfer_id"),      xferId},
        {QStringLiteral("resume_offset"),0.0},
    };
    link->sendJson(acc);
    m_events.info(QStringLiteral("file_accepted"),
                  QStringLiteral("Accepted %1 from %2").arg(in->t.fileName, in->peerId), in->peerId);
    requestNext(*in);
}

void FileTransferManager::rejectIncoming(const QString& xferId, const QString& reason)
{
    auto* in = m_incoming.value(xferId, nullptr);
    if (!in) return;
    if (auto* link = m_peers.linkFor(in->peerId)) {
        QJsonObject rej{
            {QStringLiteral("type"),    QString::fromLatin1(msg::kFileReject)},
            {QStringLiteral("xfer_id"), xferId},
            {QStringLiteral("reason"),  reason},
        };
        link->sendJson(rej);
    }
    m_repo.finish(xferId, TransferState::Cancelled);
    in->t.state = TransferState::Cancelled;
    emit transferUpdated(in->t);
    eraseIncoming(xferId);
}

void FileTransferManager::pause(const QString& xferId)
{
    if (auto* in = m_incoming.value(xferId)) {
        in->paused   = true;
        in->t.state  = TransferState::Paused;
        m_repo.updateState(xferId, TransferState::Paused);
        emit transferUpdated(in->t);
        if (auto* link = m_peers.linkFor(in->peerId)) {
            link->sendJson({{QStringLiteral("type"), QString::fromLatin1(msg::kFilePause)},
                            {QStringLiteral("xfer_id"), xferId}});
        }
    } else if (auto* o = m_outgoing.value(xferId)) {
        o->t.state = TransferState::Paused;
        m_repo.updateState(xferId, TransferState::Paused);
        emit transferUpdated(o->t);
    }
}

void FileTransferManager::resume(const QString& xferId)
{
    if (auto* in = m_incoming.value(xferId)) {
        in->paused = false;
        in->t.state = TransferState::Active;
        m_repo.updateState(xferId, TransferState::Active);
        emit transferUpdated(in->t);
        if (auto* link = m_peers.linkFor(in->peerId)) {
            link->sendJson({{QStringLiteral("type"), QString::fromLatin1(msg::kFileResume)},
                            {QStringLiteral("xfer_id"), xferId}});
        }
        requestNext(*in);
    }
}

void FileTransferManager::cancel(const QString& xferId)
{
    if (m_incoming.contains(xferId)) {
        m_repo.finish(xferId, TransferState::Cancelled);
        eraseIncoming(xferId);
    } else if (m_outgoing.contains(xferId)) {
        m_repo.finish(xferId, TransferState::Cancelled);
        eraseOutgoing(xferId);
    }
}

QList<Transfer> FileTransferManager::activeTransfers() const
{
    QList<Transfer> out;
    for (auto* o : m_outgoing.values()) out.push_back(o->t);
    for (auto* i : m_incoming.values()) out.push_back(i->t);
    return out;
}

void FileTransferManager::onPeerJson(const QString& peerId, const QJsonObject& obj)
{
    const QString type = obj.value(QStringLiteral("type")).toString();
    const QString xid  = obj.value(QStringLiteral("xfer_id")).toString();
    if (xid.isEmpty()) return;

    if (type == QString::fromLatin1(msg::kFileOffer)) {
        auto* in = new Incoming;
        in->peerId       = peerId;
        in->t.id         = xid;
        in->t.peerId     = peerId;
        in->t.direction  = TransferDirection::Incoming;
        in->t.fileName   = obj.value(QStringLiteral("name")).toString();
        in->t.fileSize   = static_cast<qint64>(obj.value(QStringLiteral("size")).toDouble());
        in->t.mime       = obj.value(QStringLiteral("mime")).toString();
        in->t.sha256     = obj.value(QStringLiteral("sha256")).toString();
        in->chunkSize    = obj.value(QStringLiteral("chunk_size")).toInt(65536);
        in->totalChunks  = obj.value(QStringLiteral("chunks")).toInt(0);
        in->t.state      = TransferState::Pending;
        in->t.localPath  = m_settings.downloadsDir() + QStringLiteral("/") + in->t.fileName;
        m_incoming.insert(xid, in);
        m_repo.upsert(in->t);
        m_events.info(QStringLiteral("file_offered"),
                      QStringLiteral("Incoming %1 from %2").arg(in->t.fileName, peerId), peerId);

        // Also write a chat message item.
        MessageRepository msgRepo(m_db);
        Message m;
        m.id        = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m.peerId    = peerId;
        m.direction = MessageDirection::Incoming;
        m.body      = QStringLiteral("📎 %1 (%2 bytes)").arg(in->t.fileName).arg(in->t.fileSize);
        m.ts        = QDateTime::currentSecsSinceEpoch();
        m.status    = MessageStatus::Delivered;
        m.kind      = MessageKind::File;
        msgRepo.insert(m);
        in->t.messageId = m.id;
        m_repo.upsert(in->t);

        emit incomingOffer(in->t);
        return;
    }

    if (type == QString::fromLatin1(msg::kFileAccept)) {
        auto* o = m_outgoing.value(xid, nullptr);
        if (!o) return;
        const qint64 resume = static_cast<qint64>(obj.value(QStringLiteral("resume_offset")).toDouble());
        o->t.bytesDone = resume;
        o->t.state = TransferState::Active;
        m_repo.upsert(o->t);
        emit transferUpdated(o->t);
        // Sender no longer needs to do anything; receiver drives via chunk_req.
        return;
    }

    if (type == QString::fromLatin1(msg::kFileReject)) {
        if (auto* o = m_outgoing.value(xid, nullptr)) {
            o->t.state = TransferState::Cancelled;
            m_repo.finish(xid, TransferState::Cancelled);
            m_events.info(QStringLiteral("file_rejected"),
                          QStringLiteral("Peer rejected %1").arg(o->t.fileName), o->peerId);
            emit transferUpdated(o->t);
            eraseOutgoing(xid);
        }
        return;
    }

    if (type == QString::fromLatin1(msg::kFileChunkReq)) {
        auto* o = m_outgoing.value(xid, nullptr);
        if (!o) return;
        const int idx = obj.value(QStringLiteral("index")).toInt();
        sendChunk(*o, idx);
        return;
    }

    if (type == QString::fromLatin1(msg::kFileChunkMeta)) {
        auto* in = m_incoming.value(xid, nullptr);
        if (!in) return;
        in->nextIndex        = obj.value(QStringLiteral("index")).toInt();
        in->pendingChunkSize = obj.value(QStringLiteral("size")).toInt();
        return;
    }

    if (type == QString::fromLatin1(msg::kFilePause)) {
        if (auto* o = m_outgoing.value(xid)) {
            o->t.state = TransferState::Paused;
            emit transferUpdated(o->t);
        }
        return;
    }

    if (type == QString::fromLatin1(msg::kFileResume)) {
        if (auto* o = m_outgoing.value(xid)) {
            o->t.state = TransferState::Active;
            emit transferUpdated(o->t);
        }
        return;
    }

    if (type == QString::fromLatin1(msg::kFileComplete)) {
        if (auto* o = m_outgoing.value(xid)) {
            const bool ok = obj.value(QStringLiteral("ok")).toBool();
            const auto state = ok ? TransferState::Done : TransferState::Failed;
            o->t.state = state;
            m_repo.finish(xid, state);
            m_events.info(QStringLiteral("file_completed"),
                          QStringLiteral("Send %1: %2").arg(o->t.fileName,
                              ok ? QStringLiteral("ok") : QStringLiteral("failed")),
                          o->peerId);
            emit transferUpdated(o->t);
            eraseOutgoing(xid);
        }
        return;
    }
}

void FileTransferManager::onPeerBinary(const QString& peerId, const QByteArray& body)
{
    Q_UNUSED(peerId);
    // Find the incoming transfer that is waiting for a chunk.
    Incoming* target = nullptr;
    for (auto* in : m_incoming) {
        if (in->peerId == peerId && in->pendingChunkSize > 0
            && in->pendingChunkSize == body.size()) {
            target = in;
            break;
        }
    }
    if (!target || !target->file) return;

    target->file->seek(static_cast<qint64>(target->nextIndex) * target->chunkSize);
    target->file->write(body);
    target->t.bytesDone += body.size();
    m_repo.updateProgress(target->t.id, target->t.bytesDone, TransferState::Active);
    emit transferUpdated(target->t);

    target->pendingChunkSize = 0;
    target->nextIndex += 1;

    if (target->nextIndex >= target->totalChunks) {
        finishIncomingOk(*target);
    } else if (!target->paused) {
        requestNext(*target);
    }
}

void FileTransferManager::sendChunk(Outgoing& o, int chunkIndex)
{
    if (!o.file) return;
    auto* link = m_peers.linkFor(o.peerId);
    if (!link) return;

    const qint64 offset = static_cast<qint64>(chunkIndex) * o.chunkSize;
    if (offset >= o.t.fileSize) return;
    o.file->seek(offset);
    QByteArray buf = o.file->read(o.chunkSize);

    QJsonObject meta{
        {QStringLiteral("type"),    QString::fromLatin1(msg::kFileChunkMeta)},
        {QStringLiteral("xfer_id"), o.t.id},
        {QStringLiteral("index"),   chunkIndex},
        {QStringLiteral("size"),    buf.size()},
    };
    link->sendJsonAndBinary(meta, buf);

    o.t.bytesDone = std::min(o.t.fileSize, offset + buf.size());
    m_repo.updateProgress(o.t.id, o.t.bytesDone, TransferState::Active);
    emit transferUpdated(o.t);
}

void FileTransferManager::requestNext(Incoming& in)
{
    auto* link = m_peers.linkFor(in.peerId);
    if (!link) return;
    QJsonObject req{
        {QStringLiteral("type"),    QString::fromLatin1(msg::kFileChunkReq)},
        {QStringLiteral("xfer_id"), in.t.id},
        {QStringLiteral("index"),   in.nextIndex},
    };
    link->sendJson(req);
}

void FileTransferManager::finishIncomingOk(Incoming& in)
{
    if (in.file) { in.file->close(); }
    QString actual = sha256OfFile(in.t.localPath);
    const bool ok = (actual == in.t.sha256);
    in.t.state = ok ? TransferState::Done : TransferState::Failed;
    m_repo.finish(in.t.id, in.t.state);
    emit transferUpdated(in.t);

    if (auto* link = m_peers.linkFor(in.peerId)) {
        QJsonObject done{
            {QStringLiteral("type"),       QString::fromLatin1(msg::kFileComplete)},
            {QStringLiteral("xfer_id"),    in.t.id},
            {QStringLiteral("ok"),         ok},
            {QStringLiteral("sha256_ok"),  ok},
        };
        link->sendJson(done);
    }
    m_events.info(QStringLiteral("file_completed"),
                  QStringLiteral("Receive %1: %2").arg(in.t.fileName,
                      ok ? QStringLiteral("ok") : QStringLiteral("checksum mismatch")),
                  in.peerId);
    eraseIncoming(in.t.id);
}

void FileTransferManager::finishIncomingFail(Incoming& in, const QString& reason)
{
    in.t.state = TransferState::Failed;
    m_repo.finish(in.t.id, TransferState::Failed);
    m_events.error(QStringLiteral("file_failed"),
                   QStringLiteral("%1: %2").arg(in.t.fileName, reason), in.peerId);
    emit transferUpdated(in.t);
    eraseIncoming(in.t.id);
}

void FileTransferManager::eraseOutgoing(const QString& id)
{
    auto* o = m_outgoing.take(id);
    if (!o) return;
    if (o->file) o->file->close();
    delete o;
}

void FileTransferManager::eraseIncoming(const QString& id)
{
    auto* in = m_incoming.take(id);
    if (!in) return;
    if (in->file) in->file->close();
    delete in;
}

} // namespace nodetalk::net
