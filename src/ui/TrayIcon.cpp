#include "TrayIcon.h"

#include "MainWindow.h"
#include "core/Settings.h"
#include "model/Message.h"
#include "model/Peer.h"
#include "net/PeerManager.h"

#include <QAction>
#include <QApplication>
#include <QMenu>

namespace nodetalk::ui {

TrayIcon::TrayIcon(MainWindow* mw, Settings& s, net::PeerManager& pm, QObject* parent)
    : QSystemTrayIcon(QIcon(QStringLiteral(":/resources/icons/nodetalk.svg")), parent),
      m_mw(mw), m_settings(s), m_peers(pm)
{
    m_menu    = new QMenu;
    m_actShow = m_menu->addAction(tr("Show"));
    m_actHide = m_menu->addAction(tr("Hide"));
    m_menu->addSeparator();
    m_actQuit = m_menu->addAction(tr("Quit"));
    setContextMenu(m_menu);
    setToolTip(QStringLiteral("NodeTalk"));

    connect(m_actShow, &QAction::triggered, this, [this]{ if (m_mw) { m_mw->showNormal(); m_mw->activateWindow(); }});
    connect(m_actHide, &QAction::triggered, this, [this]{ if (m_mw) m_mw->hide(); });
    connect(m_actQuit, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(this, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r){
        if (r == QSystemTrayIcon::Trigger && m_mw) {
            if (m_mw->isVisible()) m_mw->hide();
            else { m_mw->showNormal(); m_mw->activateWindow(); }
        }
    });

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        setVisible(true);
    }

    // Desktop notification on every incoming chat message that the user
    // is unlikely to see (window not active, or another conversation open).
    connect(&m_peers, &net::PeerManager::messageAppended,
            this, [this](const Message& m) {
        if (m.direction != MessageDirection::Incoming) return;
        const bool windowActive = m_mw && m_mw->isActiveWindow()
                                  && !m_mw->isMinimized() && m_mw->isVisible();
        const bool sameChatOpen = m_mw && m_mw->currentPeer() == m.peerId;
        if (windowActive && sameChatOpen) return;

        QString title = tr("New message");
        if (auto p = m_peers.findPeer(m.peerId)) {
            title = p->shownName();
        }
        QString body = (m.kind == MessageKind::File)
                       ? tr("\xF0\x9F\x93\x8E File: %1").arg(m.body)
                       : m.body;
        if (body.size() > 240) body = body.left(240) + QStringLiteral("\xE2\x80\xA6");
        notify(title, body);
    });
}

void TrayIcon::notify(const QString& title, const QString& body)
{
    if (!m_settings.notificationsEnabled()) return;
    if (!supportsMessages()) return;
    showMessage(title, body, QSystemTrayIcon::Information, 4000);
}

void TrayIcon::retranslateUi()
{
    if (m_actShow) m_actShow->setText(tr("Show"));
    if (m_actHide) m_actHide->setText(tr("Hide"));
    if (m_actQuit) m_actQuit->setText(tr("Quit"));
}

} // namespace nodetalk::ui
