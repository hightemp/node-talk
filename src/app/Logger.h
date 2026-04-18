#pragma once

#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(ntApp)
Q_DECLARE_LOGGING_CATEGORY(ntDb)
Q_DECLARE_LOGGING_CATEGORY(ntNet)
Q_DECLARE_LOGGING_CATEGORY(ntDisc)
Q_DECLARE_LOGGING_CATEGORY(ntXfer)
Q_DECLARE_LOGGING_CATEGORY(ntUi)

namespace nodetalk {

/// Installs the global Qt message handler that writes to a rolling log
/// file (1 MiB per file, 3 backups) and mirrors to stderr.
class Logger {
public:
    static void install(const QString& filePath);
    static void shutdown();
};

} // namespace nodetalk
