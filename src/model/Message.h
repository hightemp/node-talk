#pragma once

#include <QString>

namespace nodetalk {

enum class MessageDirection : int { Outgoing = 0, Incoming = 1 };
enum class MessageStatus    : int { Sending = 0, Sent = 1, Delivered = 2, Read = 3, Failed = 4 };
enum class MessageKind      : int { Text = 0, File = 1 };

struct Message {
    QString          id;
    QString          peerId;
    MessageDirection direction = MessageDirection::Outgoing;
    QString          body;
    qint64           ts = 0;
    MessageStatus    status = MessageStatus::Sending;
    MessageKind      kind   = MessageKind::Text;
};

} // namespace nodetalk
