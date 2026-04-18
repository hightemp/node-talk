#include "EventLog.h"

#include <QDateTime>

namespace nodetalk {

EventLog::EventLog(Database& db, QObject* parent)
    : QObject(parent), m_repo(db) {}

void EventLog::info (const QString& k, const QString& t, const QString& p) { append(0, k, t, p); }
void EventLog::warn (const QString& k, const QString& t, const QString& p) { append(1, k, t, p); }
void EventLog::error(const QString& k, const QString& t, const QString& p) { append(2, k, t, p); }

void EventLog::append(int level, const QString& kind, const QString& text, const QString& peerId)
{
    Event e;
    e.ts     = QDateTime::currentSecsSinceEpoch();
    e.level  = level;
    e.kind   = kind;
    e.peerId = peerId;
    e.text   = text;
    e.id     = m_repo.insert(e);
    emit appended(e);
}

} // namespace nodetalk
