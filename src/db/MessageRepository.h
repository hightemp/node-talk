#pragma once

#include "model/Message.h"

#include <QList>
#include <optional>

namespace nodetalk {

class Database;

class MessageRepository {
public:
    explicit MessageRepository(Database& db);

    bool insert(const Message& m);
    bool updateStatus(const QString& id, MessageStatus s);

    /// Returns up to `limit` messages with `peer_id == peerId`, newest
    /// first. Pass `beforeTs == 0` to fetch the most recent page.
    QList<Message> page(const QString& peerId, qint64 beforeTs, int limit) const;

    /// Substring search across one peer's history (newest first).
    QList<Message> search(const QString& peerId, const QString& query, int limit) const;

    std::optional<Message> findById(const QString& id) const;

private:
    Database& m_db;
};

} // namespace nodetalk
