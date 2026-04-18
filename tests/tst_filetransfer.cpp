// File-transfer protocol-level integration test.
//
// We exercise the framing layer end-to-end via TCP sockets without
// pulling up the whole PeerManager (the latter requires identity +
// discovery + DB and is covered by tst_smoke instead).

#include "net/Protocol.h"

#include <QHostAddress>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

using namespace nodetalk::net;

class TstFileTransfer : public QObject {
    Q_OBJECT
private slots:
    void framed_chunk_round_trips_over_tcp();
};

void TstFileTransfer::framed_chunk_round_trips_over_tcp()
{
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY(client.waitForConnected(2000));
    QVERIFY(server.waitForNewConnection(2000));

    QTcpSocket* peer = server.nextPendingConnection();
    QVERIFY(peer);

    // Sender: announce a chunk meta then ship its body.
    QJsonObject meta{
        {QStringLiteral("type"),  QStringLiteral("file_chunk_meta")},
        {QStringLiteral("xfer_id"), QStringLiteral("X1")},
        {QStringLiteral("index"), 0},
        {QStringLiteral("size"),  4096},
    };
    const QByteArray body(4096, '\x42');
    client.write(encodeJson(meta));
    client.write(encodeBinary(body));
    while (client.bytesToWrite() > 0) QVERIFY(client.waitForBytesWritten(2000));

    FrameReader reader;
    QElapsedTimer deadline; deadline.start();
    int got = 0;
    QByteArray gotBody;
    while (deadline.elapsed() < 5000 && got < 2) {
        peer->waitForReadyRead(200);
        const QByteArray chunk = peer->readAll();
        if (!chunk.isEmpty()) reader.append(chunk);
        for (;;) {
            auto f = reader.next();
            if (!f.valid) break;
            if (f.type == frame::kBin) gotBody = f.payload;
            ++got;
        }
    }
    QCOMPARE(got, 2);
    QCOMPARE(gotBody, body);
}

QTEST_MAIN(TstFileTransfer)
#include "tst_filetransfer.moc"
