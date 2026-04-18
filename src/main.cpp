#include "app/Application.h"
#include "app/Logger.h"
#include "app/Paths.h"
#include "app/Version.h"

#include <QApplication>
#include <QDir>

int main(int argc, char* argv[])
{
    QApplication::setApplicationName(QString::fromLatin1(nodetalk::kAppName));
    QApplication::setOrganizationName(QString::fromLatin1(nodetalk::kAppOrg));
    QApplication::setOrganizationDomain(QString::fromLatin1(nodetalk::kAppDomain));
    QApplication::setApplicationVersion(QString::fromLatin1(nodetalk::kAppVersion));
    QApplication::setQuitOnLastWindowClosed(false);

    QApplication app(argc, argv);

    QDir().mkpath(nodetalk::Paths::dataDir());
    nodetalk::Logger::install(nodetalk::Paths::logFile());

    nodetalk::app::Application application;
    if (!application.initialize()) {
        return 1;
    }
    application.show();

    const int rc = QApplication::exec();
    application.shutdown();
    return rc;
}
