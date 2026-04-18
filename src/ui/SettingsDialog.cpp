#include "SettingsDialog.h"

#include "core/Settings.h"
#include "net/PeerManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace nodetalk::ui {

SettingsDialog::SettingsDialog(Settings& s, net::PeerManager& pm, QWidget* parent)
    : QDialog(parent), m_settings(s), m_peers(pm)
{
    setWindowTitle(tr("Preferences"));
    resize(560, 420);

    auto* lay = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(buildGeneralTab(), tr("General"));
    m_tabs->addTab(buildNetworkTab(), tr("Network"));
    m_tabs->addTab(buildTrayTab(),    tr("Tray"));
    m_tabs->addTab(buildPeersTab(),   tr("Trusted peers"));
    lay->addWidget(m_tabs);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(bb);
}

QWidget* SettingsDialog::buildGeneralTab()
{
    auto* w = new QWidget(this);
    auto* form = new QFormLayout(w);

    m_displayName = new QLineEdit(m_settings.displayName(), w);
    form->addRow(tr("Display name"), m_displayName);

    m_language = new QComboBox(w);
    m_language->addItem(tr("System"), QStringLiteral("system"));
    m_language->addItem(tr("English"), QStringLiteral("en"));
    m_language->addItem(tr("Russian"), QStringLiteral("ru"));
    const int idx = m_language->findData(m_settings.language());
    m_language->setCurrentIndex(idx >= 0 ? idx : 0);
    form->addRow(tr("Language"), m_language);

    auto* row = new QHBoxLayout;
    m_downloads = new QLineEdit(m_settings.downloadsDir(), w);
    m_browse    = new QPushButton(tr("Browse…"), w);
    connect(m_browse, &QPushButton::clicked, this, &SettingsDialog::onPickFolder);
    row->addWidget(m_downloads, 1);
    row->addWidget(m_browse);
    form->addRow(tr("Default download folder"), row);

    return w;
}

QWidget* SettingsDialog::buildNetworkTab()
{
    auto* w = new QWidget(this);
    auto* form = new QFormLayout(w);

    m_discoveryPort = new QSpinBox(w);
    m_discoveryPort->setRange(1, 65535);
    m_discoveryPort->setValue(m_settings.discoveryPort());
    form->addRow(tr("Discovery port"), m_discoveryPort);

    m_tcpPort = new QSpinBox(w);
    m_tcpPort->setRange(0, 65535);
    m_tcpPort->setValue(m_settings.tcpPort());
    form->addRow(tr("TCP port (0 = auto)"), m_tcpPort);

    m_peerTimeout = new QSpinBox(w);
    m_peerTimeout->setRange(5, 600);
    m_peerTimeout->setValue(m_settings.peerTimeoutSec());
    form->addRow(tr("Peer timeout (seconds)"), m_peerTimeout);

    m_maxParallel = new QSpinBox(w);
    m_maxParallel->setRange(1, 16);
    m_maxParallel->setValue(m_settings.maxParallelTransfers());
    form->addRow(tr("Max parallel transfers"), m_maxParallel);

    return w;
}

QWidget* SettingsDialog::buildTrayTab()
{
    auto* w = new QWidget(this);
    auto* lay = new QVBoxLayout(w);

    m_startInTray   = new QCheckBox(tr("Start in tray"), w);
    m_minToTray     = new QCheckBox(tr("Minimize to tray"), w);
    m_closeToTray   = new QCheckBox(tr("Close to tray"), w);
    m_notifications = new QCheckBox(tr("Enable desktop notifications"), w);

    m_startInTray->setChecked(m_settings.startInTray());
    m_minToTray->setChecked(m_settings.minimizeToTray());
    m_closeToTray->setChecked(m_settings.closeToTray());
    m_notifications->setChecked(m_settings.notificationsEnabled());

    lay->addWidget(m_startInTray);
    lay->addWidget(m_minToTray);
    lay->addWidget(m_closeToTray);
    lay->addWidget(m_notifications);
    lay->addStretch(1);
    return w;
}

QWidget* SettingsDialog::buildPeersTab()
{
    auto* w = new QWidget(this);
    auto* lay = new QVBoxLayout(w);
    auto* tree = new QTreeWidget(w);
    tree->setColumnCount(4);
    tree->setHeaderLabels({tr("Nickname"), tr("IP"), tr("Trust"), tr("Fingerprint")});
    tree->setRootIsDecorated(false);
    tree->header()->setStretchLastSection(true);
    for (const auto& p : m_peers.allPeers()) {
        auto* it = new QTreeWidgetItem(tree);
        it->setText(0, p.shownName());
        it->setText(1, p.lastIp);
        QString trust;
        switch (p.trust) {
            case TrustState::Unknown: trust = tr("Unknown"); break;
            case TrustState::Trusted: trust = tr("Trusted"); break;
            case TrustState::Blocked: trust = tr("Blocked"); break;
        }
        it->setText(2, trust);
        it->setText(3, p.fingerprint);
        it->setData(0, Qt::UserRole, p.id);
    }
    lay->addWidget(tree, 1);

    auto* row = new QHBoxLayout;
    auto* trustBtn  = new QPushButton(tr("Trust"), w);
    auto* blockBtn  = new QPushButton(tr("Block"), w);
    auto* renameBtn = new QPushButton(tr("Rename…"), w);
    row->addWidget(trustBtn);
    row->addWidget(blockBtn);
    row->addWidget(renameBtn);
    row->addStretch(1);
    lay->addLayout(row);

    auto currentId = [tree]() -> QString {
        auto items = tree->selectedItems();
        return items.isEmpty() ? QString() : items.first()->data(0, Qt::UserRole).toString();
    };
    connect(trustBtn,  &QPushButton::clicked, this, [this, currentId]{ if (auto id = currentId(); !id.isEmpty()) m_peers.trust(id); });
    connect(blockBtn,  &QPushButton::clicked, this, [this, currentId]{ if (auto id = currentId(); !id.isEmpty()) m_peers.block(id); });
    connect(renameBtn, &QPushButton::clicked, this, [this, currentId, w]{
        const auto id = currentId();
        if (id.isEmpty()) return;
        bool ok = false;
        const QString name = QFileDialog::getSaveFileName(w, tr("Rename peer"));
        Q_UNUSED(ok);
        if (!name.isEmpty()) m_peers.rename(id, name);
    });
    return w;
}

void SettingsDialog::onAccepted()
{
    m_settings.setDisplayName(m_displayName->text().trimmed());
    m_settings.setLanguage(m_language->currentData().toString());
    m_settings.setDownloadsDir(m_downloads->text().trimmed());

    m_settings.setDiscoveryPort(m_discoveryPort->value());
    m_settings.setTcpPort(m_tcpPort->value());
    m_settings.setPeerTimeoutSec(m_peerTimeout->value());
    m_settings.setMaxParallelTransfers(m_maxParallel->value());

    m_settings.setStartInTray(m_startInTray->isChecked());
    m_settings.setMinimizeToTray(m_minToTray->isChecked());
    m_settings.setCloseToTray(m_closeToTray->isChecked());
    m_settings.setNotificationsEnabled(m_notifications->isChecked());
}

void SettingsDialog::onPickFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Default download folder"),
                                                          m_downloads->text());
    if (!dir.isEmpty()) m_downloads->setText(dir);
}

} // namespace nodetalk::ui
