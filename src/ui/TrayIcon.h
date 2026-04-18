#pragma once

#include <QSystemTrayIcon>

class QMenu;
class QAction;

namespace nodetalk { class Settings; }
namespace nodetalk::net { class PeerManager; }

namespace nodetalk::ui {

class MainWindow;

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT
public:
    TrayIcon(MainWindow* mw, Settings& s, net::PeerManager& pm, QObject* parent = nullptr);

    void notify(const QString& title, const QString& body);
    void retranslateUi();

private:
    MainWindow*       m_mw;
    Settings&         m_settings;
    net::PeerManager& m_peers;
    QMenu*            m_menu     = nullptr;
    QAction*          m_actShow  = nullptr;
    QAction*          m_actHide  = nullptr;
    QAction*          m_actQuit  = nullptr;
};

} // namespace nodetalk::ui
