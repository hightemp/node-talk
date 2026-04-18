#include "TransportServer.h"

#include "app/Logger.h"

#include <QTcpSocket>

namespace nodetalk::net {

TransportServer::TransportServer(QObject* parent) : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, [this] {
        while (auto* s = m_server.nextPendingConnection()) {
            emit incomingSocket(s);
        }
    });
}

TransportServer::~TransportServer() = default;

bool TransportServer::start(quint16 preferredPort)
{
    if (m_server.isListening()) m_server.close();
    if (!m_server.listen(QHostAddress::Any, preferredPort)) {
        qCWarning(ntNet) << "TCP listen failed:" << m_server.errorString();
        return false;
    }
    qCInfo(ntNet) << "TCP listening on port" << m_server.serverPort();
    return true;
}

void TransportServer::stop()
{
    if (m_server.isListening()) m_server.close();
}

} // namespace nodetalk::net
