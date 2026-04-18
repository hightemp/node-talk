#pragma once

#include "model/Transfer.h"

#include <QList>
#include <optional>

namespace nodetalk {

class Database;

class TransferRepository {
public:
    explicit TransferRepository(Database& db);

    bool upsert(const Transfer& t);
    bool updateProgress(const QString& id, qint64 bytesDone, TransferState s);
    bool updateState(const QString& id, TransferState s);
    bool finish(const QString& id, TransferState terminal);

    std::optional<Transfer> findById(const QString& id) const;
    QList<Transfer>         forPeer(const QString& peerId) const;
    QList<Transfer>         active() const;

private:
    Database& m_db;
};

} // namespace nodetalk
