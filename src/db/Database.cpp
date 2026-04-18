#include "Database.h"

#include "app/Logger.h"

#include <QAtomicInt>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>

namespace nodetalk {

namespace {
QAtomicInt g_counter = 0;

constexpr int kCurrentSchema = 1;

const char* const kSchemaV1[] = {
    "CREATE TABLE IF NOT EXISTS settings ("
    "  key TEXT PRIMARY KEY,"
    "  value TEXT NOT NULL)",

    "CREATE TABLE IF NOT EXISTS peers ("
    "  id TEXT PRIMARY KEY,"
    "  fingerprint TEXT NOT NULL,"
    "  nickname TEXT NOT NULL,"
    "  display_name TEXT,"
    "  trust INTEGER NOT NULL DEFAULT 0,"
    "  last_ip TEXT,"
    "  last_port INTEGER,"
    "  last_seen INTEGER,"
    "  created_at INTEGER NOT NULL)",

    "CREATE TABLE IF NOT EXISTS messages ("
    "  id TEXT PRIMARY KEY,"
    "  peer_id TEXT NOT NULL REFERENCES peers(id) ON DELETE CASCADE,"
    "  direction INTEGER NOT NULL,"
    "  body TEXT NOT NULL,"
    "  ts INTEGER NOT NULL,"
    "  status INTEGER NOT NULL,"
    "  kind INTEGER NOT NULL DEFAULT 0)",

    "CREATE INDEX IF NOT EXISTS idx_messages_peer_ts ON messages(peer_id, ts)",
    "CREATE INDEX IF NOT EXISTS idx_messages_body    ON messages(body)",

    "CREATE TABLE IF NOT EXISTS transfers ("
    "  id TEXT PRIMARY KEY,"
    "  msg_id TEXT REFERENCES messages(id) ON DELETE SET NULL,"
    "  peer_id TEXT NOT NULL REFERENCES peers(id),"
    "  direction INTEGER NOT NULL,"
    "  file_name TEXT NOT NULL,"
    "  file_size INTEGER NOT NULL,"
    "  mime TEXT,"
    "  sha256 TEXT,"
    "  local_path TEXT NOT NULL,"
    "  bytes_done INTEGER NOT NULL DEFAULT 0,"
    "  state INTEGER NOT NULL,"
    "  started_at INTEGER,"
    "  finished_at INTEGER)",

    "CREATE INDEX IF NOT EXISTS idx_transfers_peer ON transfers(peer_id)",

    "CREATE TABLE IF NOT EXISTS events ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  ts INTEGER NOT NULL,"
    "  level INTEGER NOT NULL,"
    "  kind TEXT NOT NULL,"
    "  peer_id TEXT,"
    "  text TEXT)",

    "CREATE INDEX IF NOT EXISTS idx_events_ts ON events(ts)",
};

} // namespace

Database::Database()
{
    m_connectionName = QStringLiteral("nodetalk_%1").arg(g_counter.fetchAndAddOrdered(1));
}

Database::~Database()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool Database::open(const QString& path)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        qCCritical(ntDb) << "open failed:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    q.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));

    return migrate();
}

QSqlDatabase Database::handle() const
{
    return QSqlDatabase::database(m_connectionName, /*open=*/false);
}

int Database::userVersion() const
{
    QSqlQuery q(handle());
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

void Database::setUserVersion(int v)
{
    QSqlQuery q(handle());
    q.exec(QStringLiteral("PRAGMA user_version = %1").arg(v));
}

bool Database::migrate()
{
    QSqlDatabase db = handle();
    int v = userVersion();
    if (v >= kCurrentSchema) return true;

    if (!db.transaction()) {
        qCCritical(ntDb) << "transaction failed:" << db.lastError().text();
        return false;
    }
    QSqlQuery q(db);
    if (v < 1) {
        for (const char* const ddl : kSchemaV1) {
            if (!q.exec(QString::fromLatin1(ddl))) {
                qCCritical(ntDb) << "DDL failed:" << q.lastError().text() << "for" << ddl;
                db.rollback();
                return false;
            }
        }
    }
    if (!db.commit()) {
        qCCritical(ntDb) << "commit failed:" << db.lastError().text();
        return false;
    }
    setUserVersion(kCurrentSchema);
    return true;
}

} // namespace nodetalk
