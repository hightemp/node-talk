#pragma once

#include "Protocol.h"

#include <QHostAddress>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <memory>

class QTcpSocket;

namespace nodetalk::net {

/// Per-connection wrapper. Owns one `QTcpSocket`, performs the
/// `hello` / `hello_ack` handshake and demuxes incoming JSON / BIN
/// frames into Qt signals.
class PeerLink : public QObject {
    Q_OBJECT
public:
    enum class Direction { Outgoing, Incoming };
    enum class State     { Connecting, HandshakingOut, HandshakingIn, Ready, Closing, Closed };

    /// Wrap a socket that has been accepted by `TransportServer`.
    PeerLink(QTcpSocket* socket, Direction d,
             const QString& localPeerId, const QString& localFingerprint,
             const QString& localNick, QObject* parent = nullptr);

    /// Build an outgoing link; `connectToHost` will be issued.
    PeerLink(const QString& localPeerId, const QString& localFingerprint,
             const QString& localNick, QObject* parent = nullptr);

    ~PeerLink() override;

    void connectToHost(const QHostAddress& addr, quint16 port);
    void close(const QString& reason = {});

    /// Send a JSON message. The link must be in `Ready` state.
    void sendJson(const QJsonObject& obj);

    /// Send a binary chunk. Must be Ready.
    void sendBinary(const QByteArray& body);

    /// Send a JSON metadata message immediately followed by a binary
    /// frame. The two frames are written in one syscall whenever
    /// possible to keep them adjacent on the wire.
    void sendJsonAndBinary(const QJsonObject& meta, const QByteArray& body);

    State           state() const            { return m_state; }
    Direction       direction() const        { return m_dir; }
    QString         remotePeerId() const     { return m_remotePeerId; }
    QString         remoteFingerprint() const{ return m_remoteFingerprint; }
    QString         remoteNickname() const   { return m_remoteNick; }
    QHostAddress    remoteAddress() const;
    quint16         remotePort() const;
    qint64          bytesAvailableToWrite() const;

signals:
    void ready();                          // after successful handshake
    void disconnected(const QString& reason);
    void jsonReceived(const QJsonObject& obj);
    void binaryReceived(const QByteArray& payload);
    void errorOccurred(const QString& reason);

private slots:
    void onConnected();
    void onReadyRead();
    void onSocketDisconnected();
    void onSocketError();

private:
    void sendHello();
    void handleJsonFrame(const QByteArray& body);

    QTcpSocket* m_socket = nullptr;
    Direction   m_dir;
    State       m_state = State::Closed;

    QString     m_localPeerId;
    QString     m_localFingerprint;
    QString     m_localNick;

    QString     m_remotePeerId;
    QString     m_remoteFingerprint;
    QString     m_remoteNick;

    FrameReader m_reader;
};

} // namespace nodetalk::net
