#pragma once

#include "model/Peer.h"

#include <QList>
#include <optional>

namespace nodetalk {

class Database;

class PeerRepository {
public:
    explicit PeerRepository(Database& db);

    /// Insert or update by primary key.
    bool upsert(const Peer& p);

    /// Update only the volatile fields (last_ip, last_port, last_seen).
    bool touch(const QString& id, const QString& ip, quint16 port, qint64 ts);

    bool setTrust(const QString& id, TrustState s);
    bool setDisplayName(const QString& id, const QString& name);
    bool setNickname(const QString& id, const QString& nick);

    std::optional<Peer> findById(const QString& id) const;
    QList<Peer>         all() const;

private:
    Database& m_db;
};

} // namespace nodetalk
