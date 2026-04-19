#include "app/Application.h"
#include "app/Logger.h"
#include "app/Paths.h"
#include "app/Version.h"

#include <QApplication>
#include <QDir>
#include <QLockFile>
#include <QMessageBox>

#ifdef Q_OS_UNIX
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#include <unistd.h>
static void nodetalk_crash_handler(int sig) {
    void* frames[64];
    int n = backtrace(frames, 64);
    fprintf(stderr, "\n=== NodeTalk crash: signal %d ===\n", sig);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    fflush(stderr);
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

int main(int argc, char* argv[])
{
#ifdef Q_OS_UNIX
    signal(SIGSEGV, nodetalk_crash_handler);
    signal(SIGABRT, nodetalk_crash_handler);
    signal(SIGBUS,  nodetalk_crash_handler);
#endif
    QApplication::setApplicationName(QString::fromLatin1(nodetalk::kAppName));
    QApplication::setOrganizationName(QString::fromLatin1(nodetalk::kAppOrg));
    QApplication::setOrganizationDomain(QString::fromLatin1(nodetalk::kAppDomain));
    QApplication::setApplicationVersion(QString::fromLatin1(nodetalk::kAppVersion));
    QApplication::setQuitOnLastWindowClosed(false);

    QApplication app(argc, argv);

    QDir().mkpath(nodetalk::Paths::dataDir());
    nodetalk::Logger::install(nodetalk::Paths::logFile());

    QLockFile lockFile(nodetalk::Paths::dataDir() + QStringLiteral("/nodetalk.lock"));
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock(100)) {
        QMessageBox::warning(nullptr,
            QObject::tr("NodeTalk"),
            QObject::tr("NodeTalk is already running."));
        return 0;
    }

    nodetalk::app::Application application;
    if (!application.initialize()) {
        return 1;
    }
    application.show();

    const int rc = QApplication::exec();
    application.shutdown();
    return rc;
}
