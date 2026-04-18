#include "PeerRepository.h"

#include "Database.h"
#include "app/Logger.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>

namespace nodetalk {

PeerRepository::PeerRepository(Database& db) : m_db(db) {}

bool PeerRepository::upsert(const Peer& p)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "INSERT INTO peers(id,fingerprint,nickname,display_name,trust,"
        "                  last_ip,last_port,last_seen,created_at) "
        "VALUES(:id,:fp,:nick,:dn,:trust,:ip,:port,:ls,:ca) "
        "ON CONFLICT(id) DO UPDATE SET "
        "  fingerprint=excluded.fingerprint,"
        "  nickname=excluded.nickname,"
        "  display_name=COALESCE(peers.display_name, excluded.display_name),"
        "  last_ip=excluded.last_ip,"
        "  last_port=excluded.last_port,"
        "  last_seen=excluded.last_seen"));
    q.bindValue(QStringLiteral(":id"),   p.id);
    q.bindValue(QStringLiteral(":fp"),   p.fingerprint);
    q.bindValue(QStringLiteral(":nick"), p.nickname);
    q.bindValue(QStringLiteral(":dn"),   p.displayName.isEmpty() ? QVariant() : QVariant(p.displayName));
    q.bindValue(QStringLiteral(":trust"),static_cast<int>(p.trust));
    q.bindValue(QStringLiteral(":ip"),   p.lastIp);
    q.bindValue(QStringLiteral(":port"), p.lastPort);
    q.bindValue(QStringLiteral(":ls"),   p.lastSeen);
    q.bindValue(QStringLiteral(":ca"),   p.createdAt ? p.createdAt : QDateTime::currentSecsSinceEpoch());
    if (!q.exec()) {
        qCWarning(ntDb) << "peer upsert failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PeerRepository::touch(const QString& id, const QString& ip, quint16 port, qint64 ts)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "UPDATE peers SET last_ip=:ip, last_port=:port, last_seen=:ls WHERE id=:id"));
    q.bindValue(QStringLiteral(":ip"),   ip);
    q.bindValue(QStringLiteral(":port"), port);
    q.bindValue(QStringLiteral(":ls"),   ts);
    q.bindValue(QStringLiteral(":id"),   id);
    return q.exec();
}

bool PeerRepository::setTrust(const QString& id, TrustState s)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral("UPDATE peers SET trust=:t WHERE id=:id"));
    q.bindValue(QStringLiteral(":t"),  static_cast<int>(s));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool PeerRepository::setDisplayName(const QString& id, const QString& name)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral("UPDATE peers SET display_name=:n WHERE id=:id"));
    q.bindValue(QStringLiteral(":n"),  name);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

bool PeerRepository::setNickname(const QString& id, const QString& nick)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral("UPDATE peers SET nickname=:n WHERE id=:id"));
    q.bindValue(QStringLiteral(":n"),  nick);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

static Peer fromRow(const QSqlQuery& q)
{
    Peer p;
    p.id          = q.value(0).toString();
    p.fingerprint = q.value(1).toString();
    p.nickname    = q.value(2).toString();
    p.displayName = q.value(3).toString();
    p.trust       = static_cast<TrustState>(q.value(4).toInt());
    p.lastIp      = q.value(5).toString();
    p.lastPort    = static_cast<quint16>(q.value(6).toUInt());
    p.lastSeen    = q.value(7).toLongLong();
    p.createdAt   = q.value(8).toLongLong();
    return p;
}

std::optional<Peer> PeerRepository::findById(const QString& id) const
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "SELECT id,fingerprint,nickname,display_name,trust,"
        "       last_ip,last_port,last_seen,created_at "
        "FROM peers WHERE id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next()) return std::nullopt;
    return fromRow(q);
}

QList<Peer> PeerRepository::all() const
{
    QList<Peer> out;
    QSqlQuery q(m_db.handle());
    if (!q.exec(QStringLiteral(
        "SELECT id,fingerprint,nickname,display_name,trust,"
        "       last_ip,last_port,last_seen,created_at "
        "FROM peers ORDER BY nickname COLLATE NOCASE"))) return out;
    while (q.next()) out.push_back(fromRow(q));
    return out;
}

} // namespace nodetalk
