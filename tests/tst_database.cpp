// Database / repository round-trip tests.

#include "db/Database.h"
#include "db/MessageRepository.h"
#include "db/PeerRepository.h"
#include "db/TransferRepository.h"
#include "model/Message.h"
#include "model/Peer.h"
#include "model/Transfer.h"

#include <QDateTime>
#include <QTemporaryDir>
#include <QTest>

using namespace nodetalk;

class TstDatabase : public QObject {
    Q_OBJECT
private slots:
    void open_creates_schema();
    void peers_upsert_and_trust();
    void messages_insert_and_page();
    void transfers_round_trip();

private:
    QTemporaryDir m_dir;
};

void TstDatabase::open_creates_schema()
{
    QVERIFY(m_dir.isValid());
    Database db;
    QVERIFY(db.open(m_dir.filePath(QStringLiteral("a.sqlite"))));
    QCOMPARE(db.userVersion(), 1);
}

void TstDatabase::peers_upsert_and_trust()
{
    Database db;
    QVERIFY(db.open(m_dir.filePath(QStringLiteral("b.sqlite"))));
    PeerRepository repo(db);

    Peer p;
    p.id = QStringLiteral("11111111-1111-1111-1111-111111111111");
    p.fingerprint = QStringLiteral("abcd");
    p.nickname = QStringLiteral("alice");
    p.lastIp = QStringLiteral("10.0.0.2");
    p.lastPort = 45823;
    p.trust = TrustState::Unknown;

    QVERIFY(repo.upsert(p));
    auto loaded = repo.findById(p.id);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->nickname, QStringLiteral("alice"));

    QVERIFY(repo.setTrust(p.id, TrustState::Trusted));
    loaded = repo.findById(p.id);
    QCOMPARE(loaded->trust, TrustState::Trusted);

    QCOMPARE(repo.all().size(), 1);
}

void TstDatabase::messages_insert_and_page()
{
    Database db;
    QVERIFY(db.open(m_dir.filePath(QStringLiteral("c.sqlite"))));
    PeerRepository pr(db);
    MessageRepository mr(db);

    Peer p;
    p.id = QStringLiteral("peer-1");
    p.fingerprint = QStringLiteral("fp");
    QVERIFY(pr.upsert(p));

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (int i = 0; i < 5; ++i) {
        Message m;
        m.id = QStringLiteral("m%1").arg(i);
        m.peerId = p.id;
        m.direction = MessageDirection::Outgoing;
        m.status = MessageStatus::Sent;
        m.kind = MessageKind::Text;
        m.body = QStringLiteral("hello %1").arg(i);
        m.ts = now + i;
        QVERIFY(mr.insert(m));
    }
    auto page = mr.page(p.id, 0, 10);
    QCOMPARE(page.size(), 5);
    QCOMPARE(page.first().id, QStringLiteral("m4")); // newest first

    auto results = mr.search(p.id, QStringLiteral("hello 2"), 10);
    QCOMPARE(results.size(), 1);
    QCOMPARE(results.first().id, QStringLiteral("m2"));
}

void TstDatabase::transfers_round_trip()
{
    Database db;
    QVERIFY(db.open(m_dir.filePath(QStringLiteral("d.sqlite"))));
    PeerRepository pr(db);
    TransferRepository tr(db);

    Peer p;
    p.id = QStringLiteral("peer-x");
    p.fingerprint = QStringLiteral("fp");
    QVERIFY(pr.upsert(p));

    Transfer t;
    t.id = QStringLiteral("xfer-1");
    t.peerId = p.id;
    t.direction = TransferDirection::Outgoing;
    t.state = TransferState::Pending;
    t.fileName = QStringLiteral("a.bin");
    t.fileSize = 1024;
    t.bytesDone = 0;
    t.sha256 = QStringLiteral("dead");
    QVERIFY(tr.upsert(t));

    QVERIFY(tr.updateProgress(t.id, 256));
    QVERIFY(tr.updateState(t.id, TransferState::Active));
    auto loaded = tr.findById(t.id);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->bytesDone, qint64(256));
    QCOMPARE(loaded->state, TransferState::Active);
}

QTEST_GUILESS_MAIN(TstDatabase)
#include "tst_database.moc"
