#include "TransferWidget.h"

#include "net/FileTransferManager.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QUrl>

namespace nodetalk::ui {

namespace {
constexpr int kRoleXferId    = Qt::UserRole + 0;
constexpr int kRoleLocalPath = Qt::UserRole + 1;
constexpr int kRoleState     = Qt::UserRole + 2;
} // namespace

TransferWidget::TransferWidget(net::FileTransferManager& xfer, QWidget* parent)
    : QTreeWidget(parent), m_xfer(xfer)
{
    setColumnCount(5);
    setHeaderLabels({tr("File"), tr("Direction"), tr("State"),
                     tr("Progress"), tr("Size")});
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QWidget::customContextMenuRequested,
            this, &TransferWidget::onContextMenu);

    connect(&m_xfer, &net::FileTransferManager::transferAdded,
            this, &TransferWidget::onAdded);
    connect(&m_xfer, &net::FileTransferManager::transferUpdated,
            this, &TransferWidget::onUpdated);
    connect(&m_xfer, &net::FileTransferManager::incomingOffer,
            this, &TransferWidget::onAdded);
}

QString TransferWidget::stateText(TransferState s) const
{
    switch (s) {
        case TransferState::Pending:   return tr("Pending");
        case TransferState::Active:    return tr("Active");
        case TransferState::Paused:    return tr("Paused");
        case TransferState::Done:      return tr("Done");
        case TransferState::Failed:    return tr("Failed");
        case TransferState::Cancelled: return tr("Cancelled");
    }
    return {};
}

QString TransferWidget::humanSize(qint64 bytes)
{
    if (bytes < 0) return QStringLiteral("?");
    static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 5) { v /= 1024.0; ++u; }
    if (u == 0) return QStringLiteral("%1 B").arg(bytes);
    return QStringLiteral("%1 %2")
        .arg(QLocale().toString(v, 'f', v >= 100.0 ? 0 : (v >= 10.0 ? 1 : 2)))
        .arg(QLatin1String(units[u]));
}

void TransferWidget::addOrUpdate(const Transfer& t)
{
    auto it = m_items.constFind(t.id);
    QTreeWidgetItem* item = (it == m_items.constEnd()) ? new QTreeWidgetItem(this) : it.value();
    if (it == m_items.constEnd()) m_items.insert(t.id, item);

    const int pct = t.fileSize > 0 ? static_cast<int>((100.0 * t.bytesDone) / double(t.fileSize)) : 0;

    item->setText(0, t.fileName);
    item->setToolTip(0, t.localPath.isEmpty() ? t.fileName : t.localPath);
    item->setText(1, t.direction == TransferDirection::Outgoing ? tr("Out") : tr("In"));
    item->setText(2, stateText(t.state));
    item->setText(3, QStringLiteral("%1%").arg(pct));

    const QString human = QStringLiteral("%1 / %2")
                              .arg(humanSize(t.bytesDone), humanSize(t.fileSize));
    const QString exact = tr("%1 / %2 bytes")
                              .arg(QLocale().toString(t.bytesDone),
                                   QLocale().toString(t.fileSize));
    item->setText(4, human);
    item->setToolTip(4, exact);

    item->setData(0, kRoleXferId,    t.id);
    item->setData(0, kRoleLocalPath, t.localPath);
    item->setData(0, kRoleState,     int(t.state));
}

void TransferWidget::onAdded(const Transfer& t)   { addOrUpdate(t); }
void TransferWidget::onUpdated(const Transfer& t) { addOrUpdate(t); }

void TransferWidget::onContextMenu(const QPoint& pos)
{
    QTreeWidgetItem* item = itemAt(pos);
    if (!item) return;

    const QString xferId    = item->data(0, kRoleXferId).toString();
    const QString localPath = item->data(0, kRoleLocalPath).toString();
    const auto    state     = TransferState(item->data(0, kRoleState).toInt());
    const QFileInfo fi(localPath);
    const bool fileExists = !localPath.isEmpty() && fi.exists();

    QMenu menu(this);

    QAction* actOpen = menu.addAction(tr("Open file"));
    actOpen->setEnabled(fileExists && state == TransferState::Done);

    QAction* actOpenDir = menu.addAction(tr("Open containing folder"));
    actOpenDir->setEnabled(fileExists);

    QAction* actCopyPath = menu.addAction(tr("Copy file path"));
    actCopyPath->setEnabled(!localPath.isEmpty());

    menu.addSeparator();

    QAction* actCancel = menu.addAction(tr("Cancel transfer"));
    actCancel->setEnabled(state == TransferState::Pending
                          || state == TransferState::Active
                          || state == TransferState::Paused);

    QAction* actDeleteFile = menu.addAction(tr("Delete file from disk…"));
    actDeleteFile->setEnabled(fileExists);

    QAction* actRemoveRow = menu.addAction(tr("Remove from list"));

    QAction* chosen = menu.exec(viewport()->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == actOpen) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
    } else if (chosen == actOpenDir) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absolutePath()));
    } else if (chosen == actCopyPath) {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(localPath));
    } else if (chosen == actCancel) {
        m_xfer.cancel(xferId);
    } else if (chosen == actDeleteFile) {
        const auto reply = QMessageBox::question(
            this, tr("Delete file"),
            tr("Permanently delete this file from disk?\n\n%1").arg(localPath),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
        QFile f(localPath);
        if (!f.remove()) {
            QMessageBox::warning(this, tr("Delete file"),
                                 tr("Could not delete file:\n%1").arg(f.errorString()));
            return;
        }
        // Mark row as gone but keep it for context.
        item->setToolTip(0, tr("File deleted: %1").arg(localPath));
        item->setForeground(0, palette().color(QPalette::Disabled, QPalette::Text));
    } else if (chosen == actRemoveRow) {
        m_items.remove(xferId);
        delete item;
    }
}

} // namespace nodetalk::ui
