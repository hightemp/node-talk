#include "Settings.h"

#include "app/Paths.h"
#include "db/Database.h"

#include <QHostInfo>
#include <QSqlError>
#include <QSqlQuery>

namespace nodetalk {

namespace key {
constexpr auto kPeerId       = "peer_id";
constexpr auto kFingerprint  = "fingerprint";
constexpr auto kDisplayName  = "display_name";
constexpr auto kDiscoveryPort = "net.discovery_port";
constexpr auto kMulticastPort = "net.multicast_port";
constexpr auto kMulticastGroup= "net.multicast_group";
constexpr auto kTcpPort       = "net.tcp_port";
constexpr auto kPeerTimeout   = "net.peer_timeout";
constexpr auto kMaxParallel   = "net.max_parallel_transfers";
constexpr auto kChunkSize     = "net.chunk_size";
constexpr auto kDownloadsDir  = "downloads_dir";
constexpr auto kStartInTray   = "tray.start_in_tray";
constexpr auto kMinToTray     = "tray.minimize_to_tray";
constexpr auto kCloseToTray   = "tray.close_to_tray";
constexpr auto kNotifEnabled  = "notifications.enabled";
constexpr auto kLanguage      = "i18n.language";
}

Settings::Settings(Database& db, QObject* parent)
    : QObject(parent), m_db(db)
{
}

QString Settings::get(const QString& k, const QString& def) const
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral("SELECT value FROM settings WHERE key = :k"));
    q.bindValue(QStringLiteral(":k"), k);
    if (q.exec() && q.next()) {
        return q.value(0).toString();
    }
    return def;
}

void Settings::set(const QString& k, const QString& v)
{
    QSqlQuery q(m_db.handle());
    q.prepare(QStringLiteral(
        "INSERT INTO settings(key,value) VALUES(:k,:v) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    q.bindValue(QStringLiteral(":k"), k);
    q.bindValue(QStringLiteral(":v"), v);
    if (!q.exec()) {
        // fall through; not fatal
    }
}

QString Settings::peerId() const                 { return get(QString::fromLatin1(key::kPeerId)); }
void    Settings::setPeerId(const QString& id)   { set(QString::fromLatin1(key::kPeerId), id); }
QString Settings::fingerprint() const            { return get(QString::fromLatin1(key::kFingerprint)); }
void    Settings::setFingerprint(const QString& fp){ set(QString::fromLatin1(key::kFingerprint), fp); }

QString Settings::displayName() const
{
    const QString v = get(QString::fromLatin1(key::kDisplayName));
    if (!v.isEmpty()) return v;
    const QString host = QHostInfo::localHostName();
    return host.isEmpty() ? QStringLiteral("user") : host;
}
void Settings::setDisplayName(const QString& name)
{
    set(QString::fromLatin1(key::kDisplayName), name);
    emit displayNameChanged(name);
}

int  Settings::discoveryPort() const             { return get(QString::fromLatin1(key::kDiscoveryPort), QStringLiteral("45821")).toInt(); }
void Settings::setDiscoveryPort(int v)           { set(QString::fromLatin1(key::kDiscoveryPort), QString::number(v)); emit networkSettingsChanged(); }
int  Settings::multicastPort() const             { return get(QString::fromLatin1(key::kMulticastPort), QStringLiteral("45822")).toInt(); }
QString Settings::multicastGroup() const         { return get(QString::fromLatin1(key::kMulticastGroup), QStringLiteral("239.255.42.99")); }
int  Settings::tcpPort() const                   { return get(QString::fromLatin1(key::kTcpPort), QStringLiteral("0")).toInt(); }
void Settings::setTcpPort(int v)                 { set(QString::fromLatin1(key::kTcpPort), QString::number(v)); emit networkSettingsChanged(); }
int  Settings::peerTimeoutSec() const            { return get(QString::fromLatin1(key::kPeerTimeout), QStringLiteral("20")).toInt(); }
void Settings::setPeerTimeoutSec(int v)          { set(QString::fromLatin1(key::kPeerTimeout), QString::number(v)); }
int  Settings::maxParallelTransfers() const      { return get(QString::fromLatin1(key::kMaxParallel), QStringLiteral("3")).toInt(); }
void Settings::setMaxParallelTransfers(int v)    { set(QString::fromLatin1(key::kMaxParallel), QString::number(v)); }
int  Settings::chunkSize() const                 { return get(QString::fromLatin1(key::kChunkSize), QStringLiteral("65536")).toInt(); }
void Settings::setChunkSize(int v)               { set(QString::fromLatin1(key::kChunkSize), QString::number(v)); }

QString Settings::downloadsDir() const
{
    const QString v = get(QString::fromLatin1(key::kDownloadsDir));
    return v.isEmpty() ? Paths::defaultDownloadsDir() : v;
}
void Settings::setDownloadsDir(const QString& dir)
{
    set(QString::fromLatin1(key::kDownloadsDir), dir);
    emit downloadsDirChanged(dir);
}

bool Settings::startInTray() const               { return get(QString::fromLatin1(key::kStartInTray), QStringLiteral("0")) == QStringLiteral("1"); }
void Settings::setStartInTray(bool v)            { set(QString::fromLatin1(key::kStartInTray), v ? QStringLiteral("1") : QStringLiteral("0")); }
bool Settings::minimizeToTray() const            { return get(QString::fromLatin1(key::kMinToTray), QStringLiteral("1")) == QStringLiteral("1"); }
void Settings::setMinimizeToTray(bool v)         { set(QString::fromLatin1(key::kMinToTray), v ? QStringLiteral("1") : QStringLiteral("0")); }
bool Settings::closeToTray() const               { return get(QString::fromLatin1(key::kCloseToTray), QStringLiteral("1")) == QStringLiteral("1"); }
void Settings::setCloseToTray(bool v)            { set(QString::fromLatin1(key::kCloseToTray), v ? QStringLiteral("1") : QStringLiteral("0")); }
bool Settings::notificationsEnabled() const      { return get(QString::fromLatin1(key::kNotifEnabled), QStringLiteral("1")) == QStringLiteral("1"); }
void Settings::setNotificationsEnabled(bool v)   { set(QString::fromLatin1(key::kNotifEnabled), v ? QStringLiteral("1") : QStringLiteral("0")); }

QString Settings::language() const               { return get(QString::fromLatin1(key::kLanguage), QStringLiteral("system")); }
void Settings::setLanguage(const QString& code)
{
    set(QString::fromLatin1(key::kLanguage), code);
    emit languageChanged(code);
}

} // namespace nodetalk
