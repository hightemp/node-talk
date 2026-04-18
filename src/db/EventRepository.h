#pragma once

#include <QList>
#include <QString>

namespace nodetalk {

class Database;

struct Event {
    qint64  id     = 0;
    qint64  ts     = 0;
    int     level  = 0;     // 0 info, 1 warn, 2 error
    QString kind;
    QString peerId;
    QString text;
};

class EventRepository {
public:
    explicit EventRepository(Database& db);

    /// Returns the new row id, or -1 on error.
    qint64 insert(const Event& e);

    /// Most recent first.
    QList<Event> recent(int limit) const;

private:
    Database& m_db;
};

} // namespace nodetalk
