#pragma once

#include "model/Transfer.h"

#include <QHash>
#include <QTreeWidget>

namespace nodetalk::net { class FileTransferManager; }

namespace nodetalk::ui {

/// Live transfers panel. Shows all active and recently-finished
/// transfers with progress, state and per-row Pause / Resume / Cancel
/// controls.
class TransferWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit TransferWidget(net::FileTransferManager& xfer, QWidget* parent = nullptr);

private slots:
    void onAdded(const Transfer& t);
    void onUpdated(const Transfer& t);

private:
    void addOrUpdate(const Transfer& t);
    QString stateText(TransferState s) const;

    net::FileTransferManager& m_xfer;
    QHash<QString, QTreeWidgetItem*> m_items;
};

} // namespace nodetalk::ui
