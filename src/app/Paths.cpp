#include "Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace nodetalk {

QString Paths::dataDir()
{
    const QString d = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(d);
    return d;
}

QString Paths::defaultDownloadsDir()
{
    const QString d = dataDir() + QStringLiteral("/Downloads");
    QDir().mkpath(d);
    return d;
}

QString Paths::dbFile()
{
    return dataDir() + QStringLiteral("/nodetalk.sqlite");
}

QString Paths::logFile()
{
    return dataDir() + QStringLiteral("/nodetalk.log");
}

} // namespace nodetalk
