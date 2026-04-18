#include "MessageRepository.h"

#include "Database.h"
#include "app/Logger.h"

#include <QSqlError>
#include <QSqlQuery>

namespace nodetalk {

MessageRepository::MessageRepository(Database& db) : m_db(db) {}

bool MessageRepository::insert(const Message& m)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "INSERT INTO messages(id,peer_id,direction,body,ts,status,kind) "
        "VALUES(:id,:p,:d,:b,:t,:s,:k)"));
    q.bindValue(QStringLiteral(":id"), m.id);
    q.bindValue(QStringLiteral(":p"),  m.peerId);
    q.bindValue(QStringLiteral(":d"),  static_cast<int>(m.direction));
    q.bindValue(QStringLiteral(":b"),  m.body);
    q.bindValue(QStringLiteral(":t"),  m.ts);
    q.bindValue(QStringLiteral(":s"),  static_cast<int>(m.status));
    q.bindValue(QStringLiteral(":k"),  static_cast<int>(m.kind));
    if (!q.exec()) {
        qCWarning(ntDb) << "message insert failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool MessageRepository::updateStatus(const QString& id, MessageStatus s)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral("UPDATE messages SET status=:s WHERE id=:id"));
    q.bindValue(QStringLiteral(":s"),  static_cast<int>(s));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

static Message fromRow(const QSqlQuery& q)
{
    Message m;
    m.id        = q.value(0).toString();
    m.peerId    = q.value(1).toString();
    m.direction = static_cast<MessageDirection>(q.value(2).toInt());
    m.body      = q.value(3).toString();
    m.ts        = q.value(4).toLongLong();
    m.status    = static_cast<MessageStatus>(q.value(5).toInt());
    m.kind      = static_cast<MessageKind>(q.value(6).toInt());
    return m;
}

QList<Message> MessageRepository::page(const QString& peerId, qint64 beforeTs, int limit) const
{
    QList<Message> out;
    QSqlQuery q(m_db.handle());
    if (beforeTs > 0) {
        q.prepare(QStringLiteral(
            "SELECT id,peer_id,direction,body,ts,status,kind "
            "FROM messages WHERE peer_id=:p AND ts<:t "
            "ORDER BY ts DESC LIMIT :l"));
        q.bindValue(QStringLiteral(":t"), beforeTs);
    } else {
        q.prepare(QStringLiteral(
            "SELECT id,peer_id,direction,body,ts,status,kind "
            "FROM messages WHERE peer_id=:p "
            "ORDER BY ts DESC LIMIT :l"));
    }
    q.bindValue(QStringLiteral(":p"), peerId);
    q.bindValue(QStringLiteral(":l"), limit);
    if (!q.exec()) return out;
    while (q.next()) out.push_back(fromRow(q));
    return out;
}

QList<Message> MessageRepository::search(const QString& peerId, const QString& query, int limit) const
{
    QList<Message> out;
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "SELECT id,peer_id,direction,body,ts,status,kind "
        "FROM messages WHERE peer_id=:p AND body LIKE :q "
        "ORDER BY ts DESC LIMIT :l"));
    q.bindValue(QStringLiteral(":p"), peerId);
    q.bindValue(QStringLiteral(":q"), QStringLiteral("%%%1%%").arg(query));
    q.bindValue(QStringLiteral(":l"), limit);
    if (!q.exec()) return out;
    while (q.next()) out.push_back(fromRow(q));
    return out;
}

std::optional<Message> MessageRepository::findById(const QString& id) const
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "SELECT id,peer_id,direction,body,ts,status,kind "
        "FROM messages WHERE id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next()) return std::nullopt;
    return fromRow(q);
}

} // namespace nodetalk
