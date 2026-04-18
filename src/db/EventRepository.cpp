#include "EventRepository.h"

#include "Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace nodetalk {

EventRepository::EventRepository(Database& db) : m_db(db) {}

qint64 EventRepository::insert(const Event& e)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "INSERT INTO events(ts,level,kind,peer_id,text) VALUES(:t,:l,:k,:p,:x)"));
    q.bindValue(QStringLiteral(":t"), e.ts);
    q.bindValue(QStringLiteral(":l"), e.level);
    q.bindValue(QStringLiteral(":k"), e.kind);
    q.bindValue(QStringLiteral(":p"), e.peerId.isEmpty() ? QVariant() : QVariant(e.peerId));
    q.bindValue(QStringLiteral(":x"), e.text);
    if (!q.exec()) return -1;
    return q.lastInsertId().toLongLong();
}

QList<Event> EventRepository::recent(int limit) const
{
    QList<Event> out;
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "SELECT id,ts,level,kind,peer_id,text "
        "FROM events ORDER BY id DESC LIMIT :l"));
    q.bindValue(QStringLiteral(":l"), limit);
    if (!q.exec()) return out;
    while (q.next()) {
        Event e;
        e.id     = q.value(0).toLongLong();
        e.ts     = q.value(1).toLongLong();
        e.level  = q.value(2).toInt();
        e.kind   = q.value(3).toString();
        e.peerId = q.value(4).toString();
        e.text   = q.value(5).toString();
        out.push_back(e);
    }
    return out;
}

} // namespace nodetalk
