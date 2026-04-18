#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace nodetalk {
class Database;
class Settings;
class Identity;
class EventLog;
namespace net {
class DiscoveryService;
class TransportServer;
class PeerManager;
class FileTransferManager;
}
namespace ui { class MainWindow; class TrayIcon; }

namespace app {

/// Top-level wiring object. Owns every long-lived module and exposes
/// them by const-reference. Lifetime is tied to `main()`.
class Application : public QObject {
    Q_OBJECT
public:
    Application();
    ~Application() override;

    /// Initializes the database, identity, networking and UI.
    /// Returns false on a fatal startup error.
    bool initialize();

    /// Shows the main window unless the user opted into "start in tray".
    void show();

    /// Tears every module down in the correct order.
    void shutdown();

    Database&                    db()        { return *m_db; }
    Settings&                    settings()  { return *m_settings; }
    Identity&                    identity()  { return *m_identity; }
    EventLog&                    eventLog()  { return *m_eventLog; }
    net::DiscoveryService&       discovery() { return *m_discovery; }
    net::TransportServer&        transport() { return *m_transport; }
    net::PeerManager&            peers()     { return *m_peers; }
    net::FileTransferManager&    transfers() { return *m_transfers; }

private:
    void installTranslator(const QString& languageCode);

    std::unique_ptr<Database>                  m_db;
    std::unique_ptr<Settings>                  m_settings;
    std::unique_ptr<Identity>                  m_identity;
    std::unique_ptr<EventLog>                  m_eventLog;

    std::unique_ptr<net::DiscoveryService>     m_discovery;
    std::unique_ptr<net::TransportServer>      m_transport;
    std::unique_ptr<net::PeerManager>          m_peers;
    std::unique_ptr<net::FileTransferManager>  m_transfers;

    std::unique_ptr<ui::MainWindow>            m_window;
    std::unique_ptr<ui::TrayIcon>              m_tray;

    class TranslatorBundle;
    std::unique_ptr<TranslatorBundle>          m_translators;
};

} // namespace app
} // namespace nodetalk
