#pragma once

#include <QString>

namespace nodetalk {

enum class TransferDirection : int { Outgoing = 0, Incoming = 1 };
enum class TransferState     : int {
    Pending  = 0,
    Active   = 1,
    Paused   = 2,
    Done     = 3,
    Failed   = 4,
    Cancelled = 5,
};

struct Transfer {
    QString           id;          // xfer_id (UUID)
    QString           messageId;   // associated chat message id
    QString           peerId;
    TransferDirection direction = TransferDirection::Outgoing;
    QString           fileName;
    qint64            fileSize  = 0;
    QString           mime;
    QString           sha256;
    QString           localPath;
    qint64            bytesDone = 0;
    TransferState     state     = TransferState::Pending;
    qint64            startedAt = 0;
    qint64            finishedAt = 0;
};

} // namespace nodetalk
