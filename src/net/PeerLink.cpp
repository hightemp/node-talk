#include "PeerLink.h"

#include "app/Logger.h"
#include "app/Version.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>

namespace nodetalk::net {

PeerLink::PeerLink(QTcpSocket* socket, Direction d,
                   const QString& localPeerId, const QString& localFingerprint,
                   const QString& localNick, QObject* parent)
    : QObject(parent),
      m_socket(socket),
      m_dir(d),
      m_localPeerId(localPeerId),
      m_localFingerprint(localFingerprint),
      m_localNick(localNick)
{
    m_socket->setParent(this);
    m_state = (d == Direction::Incoming) ? State::HandshakingIn : State::HandshakingOut;

    connect(m_socket, &QTcpSocket::readyRead,    this, &PeerLink::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &PeerLink::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred,this, &PeerLink::onSocketError);

    if (m_dir == Direction::Outgoing) {
        sendHello();
    }
}

PeerLink::PeerLink(const QString& localPeerId, const QString& localFingerprint,
                   const QString& localNick, QObject* parent)
    : QObject(parent),
      m_socket(new QTcpSocket(this)),
      m_dir(Direction::Outgoing),
      m_state(State::Connecting),
      m_localPeerId(localPeerId),
      m_localFingerprint(localFingerprint),
      m_localNick(localNick)
{
    connect(m_socket, &QTcpSocket::connected,    this, &PeerLink::onConnected);
    connect(m_socket, &QTcpSocket::readyRead,    this, &PeerLink::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &PeerLink::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred,this, &PeerLink::onSocketError);
}

PeerLink::~PeerLink() = default;

void PeerLink::connectToHost(const QHostAddress& addr, quint16 port)
{
    m_state = State::Connecting;
    m_socket->connectToHost(addr, port);
}

void PeerLink::close(const QString& reason)
{
    if (m_state == State::Closed) return;
    if (m_state == State::Ready) {
        QJsonObject bye{ {QStringLiteral("type"), QString::fromLatin1(msg::kBye)} };
        sendJson(bye);
    }
    m_state = State::Closing;
    if (m_socket) m_socket->disconnectFromHost();
    Q_UNUSED(reason);
}

void PeerLink::sendJson(const QJsonObject& obj)
{
    if (!m_socket) return;
    m_socket->write(encodeJson(obj));
}

void PeerLink::sendBinary(const QByteArray& body)
{
    if (!m_socket) return;
    m_socket->write(encodeBinary(body));
}

void PeerLink::sendJsonAndBinary(const QJsonObject& meta, const QByteArray& body)
{
    if (!m_socket) return;
    QByteArray buf;
    buf.reserve(64 + body.size());
    buf.append(encodeJson(meta));
    buf.append(encodeBinary(body));
    m_socket->write(buf);
}

QHostAddress PeerLink::remoteAddress() const { return m_socket ? m_socket->peerAddress() : QHostAddress(); }
quint16      PeerLink::remotePort()    const { return m_socket ? m_socket->peerPort()    : quint16(0); }
qint64       PeerLink::bytesAvailableToWrite() const { return m_socket ? m_socket->bytesToWrite() : 0; }

void PeerLink::onConnected()
{
    m_state = State::HandshakingOut;
    sendHello();
}

void PeerLink::onReadyRead()
{
    if (!m_socket) return;
    m_reader.append(m_socket->readAll());
    while (true) {
        auto f = m_reader.next();
        if (!f.valid) break;
        if (f.type == frame::kJson) {
            handleJsonFrame(f.payload);
        } else if (f.type == frame::kBin) {
            emit binaryReceived(f.payload);
        }
    }
    if (m_reader.hasError()) {
        emit errorOccurred(m_reader.errorString());
        close();
    }
}

void PeerLink::onSocketDisconnected()
{
    m_state = State::Closed;
    emit disconnected(QStringLiteral("peer disconnected"));
}

void PeerLink::onSocketError()
{
    if (!m_socket) return;
    const QString reason = m_socket->errorString();
    emit errorOccurred(reason);
    if (m_state != State::Closed) {
        m_state = State::Closed;
        emit disconnected(reason);
    }
}

void PeerLink::sendHello()
{
    QJsonObject hello{
        {QStringLiteral("type"),         QString::fromLatin1(msg::kHello)},
        {QStringLiteral("proto"),        kProtocolVersion},
        {QStringLiteral("peer_id"),      m_localPeerId},
        {QStringLiteral("fingerprint"),  m_localFingerprint},
        {QStringLiteral("nick"),         m_localNick},
        {QStringLiteral("app_version"),  QString::fromLatin1(kAppVersion)},
    };
    sendJson(hello);
}

void PeerLink::handleJsonFrame(const QByteArray& body)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit errorOccurred(QStringLiteral("malformed JSON"));
        close();
        return;
    }
    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();

    if (type == QString::fromLatin1(msg::kHello) && m_state == State::HandshakingIn) {
        m_remotePeerId      = obj.value(QStringLiteral("peer_id")).toString();
        m_remoteFingerprint = obj.value(QStringLiteral("fingerprint")).toString();
        m_remoteNick        = obj.value(QStringLiteral("nick")).toString();
        QJsonObject ack{
            {QStringLiteral("type"),         QString::fromLatin1(msg::kHelloAck)},
            {QStringLiteral("proto"),        kProtocolVersion},
            {QStringLiteral("peer_id"),      m_localPeerId},
            {QStringLiteral("fingerprint"),  m_localFingerprint},
            {QStringLiteral("nick"),         m_localNick},
            {QStringLiteral("app_version"),  QString::fromLatin1(kAppVersion)},
        };
        sendJson(ack);
        m_state = State::Ready;
        emit ready();
        return;
    }

    if (type == QString::fromLatin1(msg::kHelloAck) && m_state == State::HandshakingOut) {
        m_remotePeerId      = obj.value(QStringLiteral("peer_id")).toString();
        m_remoteFingerprint = obj.value(QStringLiteral("fingerprint")).toString();
        m_remoteNick        = obj.value(QStringLiteral("nick")).toString();
        m_state = State::Ready;
        emit ready();
        return;
    }

    if (type == QString::fromLatin1(msg::kBye)) {
        close();
        return;
    }

    if (m_state == State::Ready) {
        emit jsonReceived(obj);
    }
}

} // namespace nodetalk::net
