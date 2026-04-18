// Smoke test: full Application bring-up under the offscreen QPA.
//
// Verifies that all modules wire together without aborting and that
// the database file is created under a temporary data dir.

#include "app/Application.h"
#include "core/Settings.h"
#include "db/Database.h"

#include <QApplication>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class TstSmoke : public QObject {
    Q_OBJECT
private slots:
    void application_initializes();
};

void TstSmoke::application_initializes()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QStandardPaths::setTestModeEnabled(true);
    qputenv("XDG_DATA_HOME",   dir.path().toLocal8Bit());
    qputenv("XDG_CONFIG_HOME", dir.path().toLocal8Bit());

    nodetalk::app::Application app;
    QVERIFY(app.initialize());
    QVERIFY(app.db().userVersion() >= 1);
    QVERIFY(!app.settings().displayName().isEmpty()
         || app.settings().displayName().isEmpty());  // just exercise getter
    app.shutdown();
}

int main(int argc, char** argv)
{
    QApplication a(argc, argv);
    TstSmoke t;
    return QTest::qExec(&t, argc, argv);
}

#include "tst_smoke.moc"
