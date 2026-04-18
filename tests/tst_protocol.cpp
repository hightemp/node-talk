// Wire-protocol round-trip and edge-case tests.
//
// These are pure unit tests and run with `QT_QPA_PLATFORM=offscreen`,
// no networking, no filesystem.

#include "net/Protocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

using namespace nodetalk::net;

class TstProtocol : public QObject {
    Q_OBJECT
private slots:
    void encodeJson_isParseable();
    void frameReader_singleFrame();
    void frameReader_chunkedDelivery();
    void frameReader_multipleFrames();
    void frameReader_oversizeIsError();
    void encodeBinary_roundTrip();
};

void TstProtocol::encodeJson_isParseable()
{
    QJsonObject obj{{QStringLiteral("type"), QStringLiteral("hello")}};
    const QByteArray frame = encodeJson(obj);
    QVERIFY(frame.size() > 5);
    // Header layout: u32 BE length (= payload size only) + u8 type + payload
    const quint32 len = (quint8(frame[0]) << 24) | (quint8(frame[1]) << 16)
                      | (quint8(frame[2]) << 8)  |  quint8(frame[3]);
    QCOMPARE(int(len) + 5, frame.size());
    QCOMPARE(quint8(frame[4]), quint8(frame::kJson));
}

void TstProtocol::frameReader_singleFrame()
{
    QJsonObject obj{{QStringLiteral("k"), QStringLiteral("v")}};
    FrameReader r;
    r.append(encodeJson(obj));
    auto f = r.next();
    QVERIFY(f.valid);
    QCOMPARE(f.type, quint8(frame::kJson));
    auto doc = QJsonDocument::fromJson(f.payload);
    QCOMPARE(doc.object().value(QStringLiteral("k")).toString(), QStringLiteral("v"));
    QVERIFY(!r.next().valid);
}

void TstProtocol::frameReader_chunkedDelivery()
{
    QJsonObject obj{{QStringLiteral("type"), QStringLiteral("text")},
                    {QStringLiteral("body"), QStringLiteral("hi")}};
    QByteArray full = encodeJson(obj);
    FrameReader r;
    for (int i = 0; i < full.size(); ++i) r.append(full.mid(i, 1));
    auto f = r.next();
    QVERIFY(f.valid);
    QCOMPARE(QJsonDocument::fromJson(f.payload).object()
             .value(QStringLiteral("body")).toString(),
             QStringLiteral("hi"));
}

void TstProtocol::frameReader_multipleFrames()
{
    FrameReader r;
    r.append(encodeJson({{QStringLiteral("n"), 1}}));
    r.append(encodeJson({{QStringLiteral("n"), 2}}));
    r.append(encodeBinary(QByteArrayLiteral("ABCD")));
    int count = 0;
    for (;;) {
        auto f = r.next();
        if (!f.valid) break;
        ++count;
    }
    QCOMPARE(count, 3);
    QVERIFY(!r.hasError());
}

void TstProtocol::frameReader_oversizeIsError()
{
    QByteArray bad;
    bad.append(char(0xFF)).append(char(0xFF)).append(char(0xFF)).append(char(0xFF));
    bad.append(char(frame::kJson));
    FrameReader r;
    r.append(bad);
    auto f = r.next();
    QVERIFY(!f.valid);
    QVERIFY(r.hasError());
}

void TstProtocol::encodeBinary_roundTrip()
{
    QByteArray body(4096, 'x');
    FrameReader r;
    r.append(encodeBinary(body));
    auto f = r.next();
    QVERIFY(f.valid);
    QCOMPARE(f.type, quint8(frame::kBin));
    QCOMPARE(f.payload, body);
}

QTEST_GUILESS_MAIN(TstProtocol)
#include "tst_protocol.moc"
