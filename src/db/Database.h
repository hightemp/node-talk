#pragma once

#include <QSqlDatabase>
#include <QString>

namespace nodetalk {

/// Owns the SQLite connection and is responsible for creating /
/// migrating the schema. The database file path is passed in by the
/// caller. The connection name is unique per `Database` instance so
/// tests can run in parallel against in-memory databases.
class Database {
public:
    Database();
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    /// Opens the database at `path` (use `:memory:` for tests).
    /// Applies migrations as needed. Returns false on fatal error.
    bool open(const QString& path);

    /// Returns the underlying QSqlDatabase. Use this to run queries.
    QSqlDatabase handle() const;

    /// Current `PRAGMA user_version`.
    int userVersion() const;

    /// Public so tests can re-open if needed.
    QString connectionName() const { return m_connectionName; }

private:
    bool migrate();
    void setUserVersion(int v);

    QString m_connectionName;
};

} // namespace nodetalk
