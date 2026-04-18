#include "Logger.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <cstdio>

Q_LOGGING_CATEGORY(ntApp,  "nt.app")
Q_LOGGING_CATEGORY(ntDb,   "nt.db")
Q_LOGGING_CATEGORY(ntNet,  "nt.net")
Q_LOGGING_CATEGORY(ntDisc, "nt.disc")
Q_LOGGING_CATEGORY(ntXfer, "nt.xfer")
Q_LOGGING_CATEGORY(ntUi,   "nt.ui")

namespace nodetalk {
namespace {

constexpr qint64 kMaxLogBytes = 1 * 1024 * 1024; // 1 MiB
constexpr int    kBackups     = 3;

QMutex            g_mutex;
QString           g_path;
QFile*            g_file = nullptr;
QtMessageHandler  g_prev = nullptr;

void rotateLocked()
{
    if (!g_file) return;
    g_file->close();
    for (int i = kBackups; i >= 1; --i) {
        const QString src = i == 1 ? g_path : QStringLiteral("%1.%2").arg(g_path).arg(i - 1);
        const QString dst = QStringLiteral("%1.%2").arg(g_path).arg(i);
        QFile::remove(dst);
        if (QFile::exists(src)) {
            QFile::rename(src, dst);
        }
    }
    g_file->setFileName(g_path);
    g_file->open(QIODevice::Append | QIODevice::Text);
}

void messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
    QMutexLocker lock(&g_mutex);

    const char* lvl = "INFO";
    switch (type) {
        case QtDebugMsg:    lvl = "DEBUG"; break;
        case QtInfoMsg:     lvl = "INFO";  break;
        case QtWarningMsg:  lvl = "WARN";  break;
        case QtCriticalMsg: lvl = "ERROR"; break;
        case QtFatalMsg:    lvl = "FATAL"; break;
    }
    const QString line = QStringLiteral("%1 %2 [%3] %4")
        .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs))
        .arg(QString::fromLatin1(lvl), -5)
        .arg(QString::fromLatin1(ctx.category ? ctx.category : "default"))
        .arg(msg);

    std::fputs(qPrintable(line), stderr);
    std::fputc('\n', stderr);

    if (g_file && g_file->isOpen()) {
        if (g_file->size() >= kMaxLogBytes) {
            rotateLocked();
        }
        QTextStream ts(g_file);
        ts << line << '\n';
        ts.flush();
    }

    if (type == QtFatalMsg) {
        std::abort();
    }
}

} // namespace

void Logger::install(const QString& filePath)
{
    QMutexLocker lock(&g_mutex);
    g_path = filePath;
    g_file = new QFile(g_path);
    g_file->open(QIODevice::Append | QIODevice::Text);
    g_prev = qInstallMessageHandler(&messageHandler);
}

void Logger::shutdown()
{
    QMutexLocker lock(&g_mutex);
    qInstallMessageHandler(g_prev);
    g_prev = nullptr;
    if (g_file) {
        g_file->close();
        delete g_file;
        g_file = nullptr;
    }
}

} // namespace nodetalk
