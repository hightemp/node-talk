#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QTabWidget;

namespace nodetalk { class Settings; class Database; }
namespace nodetalk::net { class PeerManager; }

namespace nodetalk::ui {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(Settings& s, net::PeerManager& pm, QWidget* parent = nullptr);

private slots:
    void onAccepted();
    void onPickFolder();

private:
    QWidget* buildGeneralTab();
    QWidget* buildNetworkTab();
    QWidget* buildTrayTab();
    QWidget* buildPeersTab();

    Settings&         m_settings;
    net::PeerManager& m_peers;

    QTabWidget* m_tabs = nullptr;

    QLineEdit*  m_displayName = nullptr;
    QComboBox*  m_language    = nullptr;
    QLineEdit*  m_downloads   = nullptr;
    QPushButton*m_browse      = nullptr;

    QSpinBox*   m_discoveryPort = nullptr;
    QSpinBox*   m_tcpPort       = nullptr;
    QSpinBox*   m_peerTimeout   = nullptr;
    QSpinBox*   m_maxParallel   = nullptr;

    QCheckBox*  m_startInTray   = nullptr;
    QCheckBox*  m_minToTray     = nullptr;
    QCheckBox*  m_closeToTray   = nullptr;
    QCheckBox*  m_notifications = nullptr;
};

} // namespace nodetalk::ui
