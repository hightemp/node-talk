#include "PeerListWidget.h"

#include "PeerListModel.h"

#include <QContextMenuEvent>

namespace nodetalk::ui {

PeerListWidget::PeerListWidget(PeerListModel* model, QWidget* parent)
    : QListView(parent)
{
    setModel(model);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformItemSizes(true);
    setIconSize(QSize(14, 14));
    setSpacing(2);
    setAlternatingRowColors(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(this, &QListView::clicked, this, [this, model](const QModelIndex& idx) {
        emit peerActivated(model->peerIdAt(idx.row()));
    });
    connect(this, &QListView::activated, this, [this, model](const QModelIndex& idx) {
        emit peerActivated(model->peerIdAt(idx.row()));
    });
}

void PeerListWidget::contextMenuEvent(QContextMenuEvent* e)
{
    const QModelIndex idx = indexAt(e->pos());
    if (!idx.isValid()) { QListView::contextMenuEvent(e); return; }
    const QString id = model()->data(idx, PeerListModel::PeerIdRole).toString();
    emit peerContextRequested(id, e->globalPos());
}

} // namespace nodetalk::ui
