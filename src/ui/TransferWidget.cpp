#include "TransferWidget.h"

#include "net/FileTransferManager.h"

#include <QHeaderView>

namespace nodetalk::ui {

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

void TransferWidget::addOrUpdate(const Transfer& t)
{
    auto it = m_items.constFind(t.id);
    QTreeWidgetItem* item = (it == m_items.constEnd()) ? new QTreeWidgetItem(this) : it.value();
    if (it == m_items.constEnd()) m_items.insert(t.id, item);

    const int pct = t.fileSize > 0 ? static_cast<int>((100.0 * t.bytesDone) / double(t.fileSize)) : 0;
    item->setText(0, t.fileName);
    item->setText(1, t.direction == TransferDirection::Outgoing ? tr("Out") : tr("In"));
    item->setText(2, stateText(t.state));
    item->setText(3, QStringLiteral("%1%").arg(pct));
    item->setText(4, QStringLiteral("%1 / %2").arg(t.bytesDone).arg(t.fileSize));
}

void TransferWidget::onAdded(const Transfer& t)   { addOrUpdate(t); }
void TransferWidget::onUpdated(const Transfer& t) { addOrUpdate(t); }

} // namespace nodetalk::ui
