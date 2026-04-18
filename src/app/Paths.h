#pragma once

#include <QString>

namespace nodetalk {

/// Per-OS application paths. All paths use `QStandardPaths` so they work
/// correctly on Linux (`~/.local/share/NodeTalk`), macOS
/// (`~/Library/Application Support/NodeTalk`) and Windows
/// (`%APPDATA%\NodeTalk`).
class Paths {
public:
    /// Application data directory. Created on demand.
    static QString dataDir();

    /// Default downloads / received-files directory.
    static QString defaultDownloadsDir();

    /// Path to the SQLite database file.
    static QString dbFile();

    /// Path to the rolling log file.
    static QString logFile();
};

} // namespace nodetalk
