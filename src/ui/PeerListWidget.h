#pragma once

#include <QListView>

namespace nodetalk::ui {

class PeerListModel;

class PeerListWidget : public QListView {
    Q_OBJECT
public:
    explicit PeerListWidget(PeerListModel* model, QWidget* parent = nullptr);

signals:
    void peerActivated(const QString& peerId);
    void peerContextRequested(const QString& peerId, const QPoint& globalPos);

protected:
    void contextMenuEvent(QContextMenuEvent* e) override;
};

} // namespace nodetalk::ui
