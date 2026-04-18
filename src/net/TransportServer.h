#pragma once

#include <QObject>
#include <QTcpServer>

namespace nodetalk::net {

/// Listens for incoming TCP connections. Emits one signal per accepted
/// socket; ownership is transferred to the caller.
class TransportServer : public QObject {
    Q_OBJECT
public:
    explicit TransportServer(QObject* parent = nullptr);
    ~TransportServer() override;

    /// Starts listening on `preferredPort` (0 = pick any free port).
    bool start(quint16 preferredPort = 0);
    void stop();

    quint16 localPort() const { return m_server.serverPort(); }
    bool    isListening() const { return m_server.isListening(); }

signals:
    /// Emitted for each accepted incoming socket. The receiver takes
    /// ownership and is responsible for `deleteLater()`.
    void incomingSocket(QTcpSocket* socket);

private:
    QTcpServer m_server;
};

} // namespace nodetalk::net
