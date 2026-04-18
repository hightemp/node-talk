#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <variant>

namespace nodetalk::net {

/// Wire-protocol constants and codecs. The framing is a simple
/// length-prefixed envelope used by every TCP `PeerLink`.
///
/// Frame layout:
///   [ uint32 BE length ] [ uint8 type ] [ payload bytes ... ]
///
/// `type == 0x01` -> JSON UTF-8 message (see PRD §7.3)
/// `type == 0x02` -> raw binary chunk (file transfer body)
namespace frame {
constexpr quint8 kJson = 0x01;
constexpr quint8 kBin  = 0x02;
constexpr quint32 kMaxFrameBytes = 1u * 1024u * 1024u; // 1 MiB hard cap
}

/// Message type strings used in the JSON envelope `"type"` field.
namespace msg {
constexpr auto kHello       = "hello";
constexpr auto kHelloAck    = "hello_ack";
constexpr auto kBye         = "bye";
constexpr auto kText        = "text";
constexpr auto kTextAck     = "text_ack";
constexpr auto kTyping      = "typing";
constexpr auto kFileOffer   = "file_offer";
constexpr auto kFileAccept  = "file_accept";
constexpr auto kFileReject  = "file_reject";
constexpr auto kFileChunkReq = "file_chunk_req";
constexpr auto kFileChunkMeta = "file_chunk_meta";
constexpr auto kFilePause   = "file_pause";
constexpr auto kFileResume  = "file_resume";
constexpr auto kFileComplete = "file_complete";

// Discovery (UDP) only:
constexpr auto kAnnounce    = "announce";
}

/// Encodes a JSON object into a complete frame ready for `write()`.
QByteArray encodeJson(const QJsonObject& obj);

/// Encodes a binary payload into a complete frame ready for `write()`.
QByteArray encodeBinary(const QByteArray& body);

/// Streaming frame parser. Feed bytes as they arrive; call `next()` to
/// pop fully-assembled frames.
class FrameReader {
public:
    struct Frame {
        quint8     type = 0;
        QByteArray payload;
        bool       valid = false;
    };

    void append(const QByteArray& bytes);

    /// Returns the next complete frame, or `valid == false` if not ready.
    Frame next();

    bool hasError() const { return m_error; }
    QString errorString() const { return m_errorString; }

private:
    QByteArray m_buf;
    bool       m_error = false;
    QString    m_errorString;
};

} // namespace nodetalk::net
