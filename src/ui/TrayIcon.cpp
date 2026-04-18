#include "TrayIcon.h"

#include "MainWindow.h"
#include "core/Settings.h"
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
