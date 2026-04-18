#include "Protocol.h"

#include <QJsonDocument>
#include <QtEndian>

namespace nodetalk::net {

static QByteArray encodeFrame(quint8 type, const QByteArray& body)
{
    QByteArray out;
    out.resize(5 + body.size());
    const quint32 len = static_cast<quint32>(body.size());
    qToBigEndian(len, reinterpret_cast<uchar*>(out.data()));
    out[4] = static_cast<char>(type);
    if (!body.isEmpty()) {
        std::memcpy(out.data() + 5, body.constData(), static_cast<size_t>(body.size()));
    }
    return out;
}

QByteArray encodeJson(const QJsonObject& obj)
{
    return encodeFrame(frame::kJson, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QByteArray encodeBinary(const QByteArray& body)
{
    return encodeFrame(frame::kBin, body);
}

void FrameReader::append(const QByteArray& bytes)
{
    if (m_error) return;
    m_buf.append(bytes);
}

FrameReader::Frame FrameReader::next()
{
    Frame f;
    if (m_error) return f;
    if (m_buf.size() < 5) return f;

    const quint32 len = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_buf.constData()));
    if (len > frame::kMaxFrameBytes) {
        m_error = true;
        m_errorString = QStringLiteral("frame too large: %1").arg(len);
        return f;
    }
    if (static_cast<quint32>(m_buf.size()) < 5u + len) return f;

    f.type    = static_cast<quint8>(m_buf[4]);
    f.payload = m_buf.mid(5, static_cast<int>(len));
    f.valid   = true;
    m_buf.remove(0, static_cast<int>(5u + len));
    return f;
}

} // namespace nodetalk::net
