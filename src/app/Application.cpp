#include "Application.h"
#include "Logger.h"
#include "Paths.h"

#include "core/EventLog.h"
#include "core/Identity.h"
#include "core/Settings.h"
#include "db/Database.h"
#include "net/DiscoveryService.h"
#include "net/FileTransferManager.h"
#include "net/PeerManager.h"
#include "net/TransportServer.h"
#include "ui/MainWindow.h"
#include "ui/TrayIcon.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSystemTrayIcon>
#include <QTranslator>

namespace nodetalk::app {

class Application::TranslatorBundle {
public:
    QTranslator app;
    QTranslator qt;
};

Application::Application()
    : m_translators(std::make_unique<TranslatorBundle>())
{
}

Application::~Application() = default;

bool Application::initialize()
{
    qCInfo(ntApp) << "Initializing NodeTalk";

    m_db = std::make_unique<Database>();
    if (!m_db->open(Paths::dbFile())) {
        qCCritical(ntApp) << "Failed to open database at" << Paths::dbFile();
        return false;
    }

    m_settings = std::make_unique<Settings>(*m_db);
    m_identity = std::make_unique<Identity>(*m_settings);
    m_identity->ensure();

    m_eventLog = std::make_unique<EventLog>(*m_db);

    installTranslator(m_settings->language());

    m_transport = std::make_unique<net::TransportServer>();
    if (!m_transport->start(static_cast<quint16>(m_settings->tcpPort()))) {
        qCCritical(ntApp) << "Failed to start TCP transport";
        return false;
    }

    m_discovery = std::make_unique<net::DiscoveryService>(
        *m_identity, *m_settings, m_transport->localPort());

    m_peers = std::make_unique<net::PeerManager>(
        *m_db, *m_identity, *m_settings, *m_eventLog,
        *m_transport, *m_discovery);

    m_transfers = std::make_unique<net::FileTransferManager>(
        *m_db, *m_settings, *m_eventLog, *m_peers);

    if (!m_discovery->start()) {
        qCWarning(ntApp) << "Discovery service failed to start";
    }

    m_window = std::make_unique<ui::MainWindow>(*this);
    m_tray   = std::make_unique<ui::TrayIcon>(m_window.get(), *m_settings, *m_peers);

    QObject::connect(m_settings.get(), &Settings::languageChanged,
                     this, [this](const QString& code) {
        installTranslator(code);
    });

    return true;
}

void Application::show()
{
    if (!m_window) return;
    const bool trayOk = m_tray && QSystemTrayIcon::isSystemTrayAvailable();
    if (!m_settings->startInTray() || !trayOk) {
        m_window->show();
        m_window->raise();
        m_window->activateWindow();
    }
}

void Application::shutdown()
{
    qCInfo(ntApp) << "Shutting down NodeTalk";
    if (m_discovery) m_discovery->stop();
    if (m_transport) m_transport->stop();
    m_window.reset();
    m_tray.reset();
    m_transfers.reset();
    m_peers.reset();
    m_discovery.reset();
    m_transport.reset();
    m_eventLog.reset();
    m_identity.reset();
    m_settings.reset();
    m_db.reset();
    Logger::shutdown();
}

void Application::installTranslator(const QString& languageCode)
{
    QApplication::removeTranslator(&m_translators->app);
    QApplication::removeTranslator(&m_translators->qt);

    QString code = languageCode;
    if (code.isEmpty() || code == QStringLiteral("system")) {
        code = QLocale::system().name().left(2);
    }

    if (m_translators->app.load(QStringLiteral(":/i18n/nodetalk_%1.qm").arg(code))) {
        QApplication::installTranslator(&m_translators->app);
    } else if (m_translators->app.load(QStringLiteral(":/i18n/nodetalk_en.qm"))) {
        QApplication::installTranslator(&m_translators->app);
    }

    if (m_translators->qt.load(QStringLiteral("qt_%1").arg(code),
            QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&m_translators->qt);
    }
}

} // namespace nodetalk::app
