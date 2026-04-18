#pragma once

#include <QObject>
#include <QString>
#include <QVariant>

namespace nodetalk {

class Database;

/// Strongly-typed wrapper over the `settings(key, value)` table.
/// All getters return a sane default if the key is not present.
class Settings : public QObject {
    Q_OBJECT
public:
    explicit Settings(Database& db, QObject* parent = nullptr);

    // Generic
    QString get(const QString& key, const QString& defaultValue = {}) const;
    void    set(const QString& key, const QString& value);

    // Identity
    QString peerId() const;
    void    setPeerId(const QString& id);
    QString fingerprint() const;
    void    setFingerprint(const QString& fp);
    QString displayName() const;
    void    setDisplayName(const QString& name);

    // Network
    int  discoveryPort() const;     // default 45821
    void setDiscoveryPort(int v);
    int  multicastPort() const;     // default 45822
    QString multicastGroup() const; // default 239.255.42.99
    int  tcpPort() const;           // 0 = auto
    void setTcpPort(int v);
    int  peerTimeoutSec() const;    // default 20
    void setPeerTimeoutSec(int v);
    int  maxParallelTransfers() const; // default 3
    void setMaxParallelTransfers(int v);
    int  chunkSize() const;         // default 65536
    void setChunkSize(int v);

    // Storage
    QString downloadsDir() const;
    void    setDownloadsDir(const QString& dir);

    // UX
    bool startInTray() const;
    void setStartInTray(bool v);
    bool minimizeToTray() const;
    void setMinimizeToTray(bool v);
    bool closeToTray() const;
    void setCloseToTray(bool v);
    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool v);

    // i18n: "system", "en", "ru"
    QString language() const;
    void    setLanguage(const QString& code);

signals:
    void languageChanged(const QString& code);
    void downloadsDirChanged(const QString& dir);
    void networkSettingsChanged();
    void displayNameChanged(const QString& name);

private:
    Database& m_db;
};

} // namespace nodetalk
