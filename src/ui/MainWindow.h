#pragma once

#include <QMainWindow>

class QAction;
class QLabel;
class QLineEdit;
class QMenu;

namespace nodetalk { struct Transfer; }
namespace nodetalk::app { class Application; }
namespace nodetalk::net { class FileTransferManager; class PeerManager; }
namespace nodetalk::ui {

class ChatView;
class EventLogWidget;
class MessageInput;
class PeerListModel;
class PeerListWidget;
class TransferWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(app::Application& app, QWidget* parent = nullptr);
    ~MainWindow() override;

    /// Re-applies user-visible strings (menu/labels/titles).
    void retranslateUi();

    /// The peer whose conversation is currently displayed (may be empty).
    QString currentPeer() const { return m_currentPeer; }

protected:
    void changeEvent(QEvent* e)  override;
    void closeEvent (QCloseEvent* e) override;

private slots:
    void onPeerSelected(const QString& peerId);
    void onAddPeerByIp();
    void onOpenSettings();
    void onAttachClicked();
    void onSendText(const QString& text);
    void onTrustPrompt(const QString& peerId);
    void onIncomingOffer(const ::nodetalk::Transfer& t);
    void onAbout();

private:
    void buildMenus();
    void buildActions();
    void buildLayout();
    void wireSignals();

    app::Application& m_app;

    PeerListModel*  m_peerModel = nullptr;
    PeerListWidget* m_peerList  = nullptr;
    ChatView*       m_chat      = nullptr;
    MessageInput*   m_input     = nullptr;
    TransferWidget* m_xferPanel = nullptr;
    EventLogWidget* m_logPanel  = nullptr;

    QAction *m_actAdd = nullptr, *m_actSettings = nullptr,
            *m_actQuit = nullptr, *m_actAbout = nullptr,
            *m_actSearch = nullptr;
    QMenu   *m_menuFile = nullptr, *m_menuPeers = nullptr,
            *m_menuView = nullptr, *m_menuHelp = nullptr;
    QLabel  *m_typing = nullptr;
    QLineEdit* m_searchBar = nullptr;

    QString m_currentPeer;
};

} // namespace nodetalk::ui
