#pragma once

#include "db/EventRepository.h"

#include <QObject>

namespace nodetalk {

class Database;

/// Thin facade over `EventRepository` that emits a Qt signal whenever
/// a new event is appended, so UI panels can update live.
class EventLog : public QObject {
    Q_OBJECT
public:
    explicit EventLog(Database& db, QObject* parent = nullptr);

    void info (const QString& kind, const QString& text, const QString& peerId = {});
    void warn (const QString& kind, const QString& text, const QString& peerId = {});
    void error(const QString& kind, const QString& text, const QString& peerId = {});

    EventRepository& repo() { return m_repo; }

signals:
    void appended(const Event& e);

private:
    void append(int level, const QString& kind, const QString& text, const QString& peerId);

    EventRepository m_repo;
};

} // namespace nodetalk
