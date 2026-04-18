// Identity persistence test.

#include "core/Identity.h"
#include "core/Settings.h"
#include "db/Database.h"

#include <QTemporaryDir>
#include <QTest>

using namespace nodetalk;

class TstIdentity : public QObject {
    Q_OBJECT
private slots:
    void identity_is_stable_across_reload();
};

void TstIdentity::identity_is_stable_across_reload()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString dbPath = dir.filePath(QStringLiteral("id.sqlite"));

    QString id1, fp1;
    {
        Database db;
        QVERIFY(db.open(dbPath));
        Settings s(db);
        Identity i(s);
        i.ensure();
        id1 = i.peerId();
        fp1 = i.fingerprint();
        QVERIFY(!id1.isEmpty());
        QCOMPARE(fp1.size(), 64); // 32 bytes hex
    }
    {
        Database db;
        QVERIFY(db.open(dbPath));
        Settings s(db);
        Identity i(s);
        i.ensure();
        QCOMPARE(i.peerId(), id1);
        QCOMPARE(i.fingerprint(), fp1);
    }
}

QTEST_GUILESS_MAIN(TstIdentity)
#include "tst_identity.moc"
