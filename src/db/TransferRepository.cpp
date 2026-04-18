#include "TransferRepository.h"

#include "Database.h"
#include "app/Logger.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

namespace nodetalk {

TransferRepository::TransferRepository(Database& db) : m_db(db) {}

bool TransferRepository::upsert(const Transfer& t)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "INSERT INTO transfers(id,msg_id,peer_id,direction,file_name,file_size,"
        "                      mime,sha256,local_path,bytes_done,state,started_at,finished_at) "
        "VALUES(:id,:m,:p,:d,:n,:sz,:mi,:sh,:lp,:bd,:st,:sa,:fa) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  bytes_done=excluded.bytes_done,"
        "  state=excluded.state,"
        "  local_path=excluded.local_path,"
        "  finished_at=excluded.finished_at"));
    q.bindValue(QStringLiteral(":id"), t.id);
    q.bindValue(QStringLiteral(":m"),  t.messageId.isEmpty() ? QVariant() : QVariant(t.messageId));
    q.bindValue(QStringLiteral(":p"),  t.peerId);
    q.bindValue(QStringLiteral(":d"),  static_cast<int>(t.direction));
    q.bindValue(QStringLiteral(":n"),  t.fileName);
    q.bindValue(QStringLiteral(":sz"), t.fileSize);
    q.bindValue(QStringLiteral(":mi"), t.mime);
    q.bindValue(QStringLiteral(":sh"), t.sha256);
    q.bindValue(QStringLiteral(":lp"), t.localPath);
    q.bindValue(QStringLiteral(":bd"), t.bytesDone);
    q.bindValue(QStringLiteral(":st"), static_cast<int>(t.state));
    q.bindValue(QStringLiteral(":sa"), t.startedAt);
    q.bindValue(QStringLiteral(":fa"), t.finishedAt);
    if (!q.exec()) {
        qCWarning(ntDb) << "transfer upsert failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool TransferRepository::updateProgress(const QString& id, qint64 bytesDone, TransferState s)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "UPDATE transfers SET bytes_done=:b, state=:s WHERE id=:id"));
    q.bindValue(QStringLiteral(":b"),  bytesDone);
    q.bindValue(QStringLiteral(":s"),  static_cast<int>(s));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool TransferRepository::updateState(const QString& id, TransferState s)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral("UPDATE transfers SET state=:s WHERE id=:id"));
    q.bindValue(QStringLiteral(":s"),  static_cast<int>(s));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool TransferRepository::finish(const QString& id, TransferState terminal)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "UPDATE transfers SET state=:s, finished_at=:f WHERE id=:id"));
    q.bindValue(QStringLiteral(":s"),  static_cast<int>(terminal));
    q.bindValue(QStringLiteral(":f"),  QDateTime::currentSecsSinceEpoch());
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

static Transfer fromRow(const QSqlQuery& q)
{
    Transfer t;
    t.id         = q.value(0).toString();
    t.messageId  = q.value(1).toString();
    t.peerId     = q.value(2).toString();
    t.direction  = static_cast<TransferDirection>(q.value(3).toInt());
    t.fileName   = q.value(4).toString();
    t.fileSize   = q.value(5).toLongLong();
    t.mime       = q.value(6).toString();
    t.sha256     = q.value(7).toString();
    t.localPath  = q.value(8).toString();
    t.bytesDone  = q.value(9).toLongLong();
    t.state      = static_cast<TransferState>(q.value(10).toInt());
    t.startedAt  = q.value(11).toLongLong();
    t.finishedAt = q.value(12).toLongLong();
    return t;
}

std::optional<Transfer> TransferRepository::findById(const QString& id) const
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "SELECT id,msg_id,peer_id,direction,file_name,file_size,mime,sha256,"
        "       local_path,bytes_done,state,started_at,finished_at "
        "FROM transfers WHERE id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next()) return std::nullopt;
    return fromRow(q);
}

QList<Transfer> TransferRepository::forPeer(const QString& peerId) const
{
    QList<Transfer> out;
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "SELECT id,msg_id,peer_id,direction,file_name,file_size,mime,sha256,"
        "       local_path,bytes_done,state,started_at,finished_at "
        "FROM transfers WHERE peer_id=:p ORDER BY started_at DESC"));
    q.bindValue(QStringLiteral(":p"), peerId);
    if (!q.exec()) return out;
    while (q.next()) out.push_back(fromRow(q));
    return out;
}

QList<Transfer> TransferRepository::active() const
{
    QList<Transfer> out;
    QSqlQuery q(m_db.handle());
    if (!q.exec(QStringLiteral(
        "SELECT id,msg_id,peer_id,direction,file_name,file_size,mime,sha256,"
        "       local_path,bytes_done,state,started_at,finished_at "
        "FROM transfers WHERE state IN (0,1,2) ORDER BY started_at DESC"))) return out;
    while (q.next()) out.push_back(fromRow(q));
    return out;
}

} // namespace nodetalk
