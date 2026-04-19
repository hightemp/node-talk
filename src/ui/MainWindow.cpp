#include "MainWindow.h"

#include "ChatView.h"
#include "EventLogWidget.h"
#include "ManualAddPeerDialog.h"
#include "MessageInput.h"
#include "PeerListModel.h"
#include "PeerListWidget.h"
#include "SettingsDialog.h"
#include "TransferWidget.h"
#include "TrustDialog.h"

#include "app/Application.h"
#include "app/Version.h"
#include "core/EventLog.h"
#include "core/Settings.h"
#include "model/Transfer.h"
#include "net/FileTransferManager.h"
#include "net/PeerManager.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace nodetalk::ui {

MainWindow::MainWindow(app::Application& app, QWidget* parent)
    : QMainWindow(parent), m_app(app)
{
    setWindowTitle(QStringLiteral("NodeTalk"));
    setWindowIcon(QIcon(QStringLiteral(":/resources/icons/nodetalk.svg")));
    resize(1024, 680);

    buildLayout();
    buildActions();
    buildMenus();
    wireSignals();

    statusBar()->showMessage(tr("Ready"), 2000);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildLayout()
{
    m_peerModel = new PeerListModel(m_app.peers(), this);
    m_peerList  = new PeerListWidget(m_peerModel, this);

    m_chat = new ChatView(m_app.db(), m_app.peers(), m_app.transfers(), this);
    m_input = new MessageInput(this);

    m_typing = new QLabel(this);
    m_typing->setStyleSheet(QStringLiteral("color: gray;"));

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText(tr("Search in chat… (Esc to clear)"));
    m_searchBar->setClearButtonEnabled(true);
    m_searchBar->hide();

    auto* right = new QWidget(this);
    auto* rlay = new QVBoxLayout(right);
    rlay->setContentsMargins(0, 0, 0, 0);
    rlay->addWidget(m_searchBar);
    rlay->addWidget(m_typing);
    rlay->addWidget(m_chat, 1);
    rlay->addWidget(m_input);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_peerList);
    splitter->addWidget(right);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({240, 780});
    setCentralWidget(splitter);

    m_xferPanel = new TransferWidget(m_app.transfers(), this);
    auto* xferDock = new QDockWidget(tr("Transfers"), this);
    xferDock->setObjectName(QStringLiteral("dock_transfers"));
    xferDock->setWidget(m_xferPanel);
    addDockWidget(Qt::BottomDockWidgetArea, xferDock);

    m_logPanel = new EventLogWidget(m_app.eventLog(), this);
    auto* logDock = new QDockWidget(tr("Event log"), this);
    logDock->setObjectName(QStringLiteral("dock_events"));
    logDock->setWidget(m_logPanel);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);
    tabifyDockWidget(xferDock, logDock);
    xferDock->raise();
}

void MainWindow::buildActions()
{
    m_actAdd = new QAction(tr("Add peer by IP…"), this);
    m_actAdd->setShortcut(QKeySequence(QStringLiteral("Ctrl+N")));
    connect(m_actAdd, &QAction::triggered, this, &MainWindow::onAddPeerByIp);

    m_actSettings = new QAction(tr("Preferences…"), this);
    m_actSettings->setShortcut(QKeySequence::Preferences);
    connect(m_actSettings, &QAction::triggered, this, &MainWindow::onOpenSettings);

    m_actQuit = new QAction(tr("Quit"), this);
    m_actQuit->setShortcut(QKeySequence::Quit);
    connect(m_actQuit, &QAction::triggered, qApp, &QCoreApplication::quit);

    m_actAbout = new QAction(tr("About NodeTalk"), this);
    connect(m_actAbout, &QAction::triggered, this, &MainWindow::onAbout);

    m_actSearch = new QAction(tr("Search in chat…"), this);
    m_actSearch->setShortcut(QKeySequence::Find);
    connect(m_actSearch, &QAction::triggered, this, [this]{
        m_searchBar->show();
        m_searchBar->setFocus();
        m_searchBar->selectAll();
    });
}

void MainWindow::buildMenus()
{
    m_menuFile = menuBar()->addMenu(tr("&File"));
    m_menuFile->addAction(m_actSettings);
    m_menuFile->addSeparator();
    m_menuFile->addAction(m_actQuit);

    m_menuPeers = menuBar()->addMenu(tr("&Peers"));
    m_menuPeers->addAction(m_actAdd);

    m_menuView = menuBar()->addMenu(tr("&View"));
    m_menuView->addAction(m_actSearch);

    m_menuHelp = menuBar()->addMenu(tr("&Help"));
    m_menuHelp->addAction(m_actAbout);
}

void MainWindow::wireSignals()
{
    connect(m_peerList, &PeerListWidget::peerActivated,
            this, &MainWindow::onPeerSelected);

    connect(m_input, &MessageInput::sendRequested,
            this, &MainWindow::onSendText);
    connect(m_input, &MessageInput::typingStateChanged, this, [this](bool active){
        if (!m_currentPeer.isEmpty()) m_app.peers().sendTyping(m_currentPeer, active);
    });
    connect(m_input, &MessageInput::attachRequested,
            this, &MainWindow::onAttachClicked);
    connect(m_input, &MessageInput::filesDropped, this, [this](const QStringList& paths){
        if (m_currentPeer.isEmpty()) return;
        for (const auto& p : paths) m_app.transfers().queueOutgoing(m_currentPeer, p);
    });

    connect(&m_app.peers(), &net::PeerManager::trustPromptRequested,
            this, [this](const Peer& p){ enqueueTrustPrompt(p.id); });

    connect(&m_app.peers(), &net::PeerManager::typingChanged,
            this, [this](const QString& peerId, bool active){
        if (peerId == m_currentPeer)
            m_typing->setText(active ? tr("typing…") : QString());
    });

    connect(&m_app.peers(), &net::PeerManager::peerOnlineChanged,
            this, [this](const QString& peerId, bool /*online*/){
        if (peerId == m_currentPeer) onPeerSelected(m_currentPeer);  // refresh input enable
    });

    connect(&m_app.transfers(), &net::FileTransferManager::incomingOffer,
            this, &MainWindow::onIncomingOffer);

    // Inline search bar -> live filter; Esc closes and clears.
    connect(m_searchBar, &QLineEdit::textChanged, this, [this](const QString& q){
        m_chat->setSearchFilter(q);
    });
    auto* escClose = new QShortcut(QKeySequence(Qt::Key_Escape), m_searchBar);
    escClose->setContext(Qt::WidgetShortcut);
    connect(escClose, &QShortcut::activated, this, [this]{
        m_searchBar->clear();   // triggers textChanged("") -> filter cleared
        m_searchBar->hide();
        m_chat->setFocus();
    });
}

void MainWindow::onPeerSelected(const QString& peerId)
{
    m_currentPeer = peerId;
    m_chat->setPeer(peerId);
    m_typing->clear();
    const bool canSend = m_app.peers().isOnline(peerId);
    m_input->setEnabledForSend(canSend);
    if (auto p = m_app.peers().findPeer(peerId)) {
        statusBar()->showMessage(p->shownName());
    }
}

void MainWindow::onAddPeerByIp()
{
    ManualAddPeerDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        m_app.peers().connectTo(dlg.address(), dlg.port());
    }
}

void MainWindow::onOpenSettings()
{
    SettingsDialog dlg(m_app.settings(), m_app.peers(), this);
    dlg.exec();
}

void MainWindow::onAttachClicked()
{
    if (m_currentPeer.isEmpty()) return;
    const auto files = QFileDialog::getOpenFileNames(this, tr("Send files"));
    for (const auto& f : files) m_app.transfers().queueOutgoing(m_currentPeer, f);
}

void MainWindow::onSendText(const QString& text)
{
    if (m_currentPeer.isEmpty()) return;
    m_app.peers().sendText(m_currentPeer, text);
}

void MainWindow::enqueueTrustPrompt(const QString& peerId)
{
    if (peerId.isEmpty()) return;
    if (m_trustQueued.contains(peerId)) return;          // dedupe
    if (auto p = m_app.peers().findPeer(peerId); !p || p->trust != TrustState::Unknown)
        return;                                          // already decided
    m_trustQueued.insert(peerId);
    m_trustQueue.enqueue(peerId);
    if (!m_trustPromptActive)
        QMetaObject::invokeMethod(this, &MainWindow::processNextTrustPrompt,
                                  Qt::QueuedConnection);
}

void MainWindow::processNextTrustPrompt()
{
    if (m_trustPromptActive) return;
    if (m_trustQueue.isEmpty()) return;
    const QString peerId = m_trustQueue.dequeue();
    m_trustQueued.remove(peerId);
    m_trustPromptActive = true;
    onTrustPrompt(peerId);
    m_trustPromptActive = false;
    if (!m_trustQueue.isEmpty())
        QMetaObject::invokeMethod(this, &MainWindow::processNextTrustPrompt,
                                  Qt::QueuedConnection);
}

void MainWindow::onTrustPrompt(const QString& peerId)
{
    auto pOpt = m_app.peers().findPeer(peerId);
    if (!pOpt) return;
    if (pOpt->trust != TrustState::Unknown) return;   // already decided
    TrustDialog dlg(*pOpt, this);
    const int rc = dlg.exec();
    if (rc == QDialog::Accepted) m_app.peers().trust(peerId);
    else                          m_app.peers().block(peerId);
}

void MainWindow::onIncomingOffer(const Transfer& t)
{
    const auto resp = QMessageBox::question(this, tr("Incoming file"),
        tr("Receive \"%1\" (%2 bytes)?").arg(t.fileName).arg(t.fileSize),
        QMessageBox::Yes | QMessageBox::No);
    if (resp == QMessageBox::Yes) m_app.transfers().acceptIncoming(t.id);
    else                          m_app.transfers().rejectIncoming(t.id);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About NodeTalk"),
        tr("NodeTalk %1\n\nA serverless LAN messenger.\n\nProtocol v%2")
            .arg(QString::fromLatin1(nodetalk::kAppVersionFull))
            .arg(nodetalk::kProtocolVersion));
}

void MainWindow::changeEvent(QEvent* e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) retranslateUi();
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    if (m_app.settings().closeToTray()) {
        hide();
        e->ignore();
        return;
    }
    QMainWindow::closeEvent(e);
}

void MainWindow::retranslateUi()
{
    if (m_actAdd)      m_actAdd->setText(tr("Add peer by IP…"));
    if (m_actSettings) m_actSettings->setText(tr("Preferences…"));
    if (m_actQuit)     m_actQuit->setText(tr("Quit"));
    if (m_actAbout)    m_actAbout->setText(tr("About NodeTalk"));
    if (m_actSearch)   m_actSearch->setText(tr("Search in chat…"));
    if (m_menuFile)    m_menuFile->setTitle(tr("&File"));
    if (m_menuPeers)   m_menuPeers->setTitle(tr("&Peers"));
    if (m_menuView)    m_menuView->setTitle(tr("&View"));
    if (m_menuHelp)    m_menuHelp->setTitle(tr("&Help"));
}

} // namespace nodetalk::ui
