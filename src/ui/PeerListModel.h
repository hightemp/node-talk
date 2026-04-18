#pragma once

#include "model/Peer.h"

#include <QAbstractListModel>
#include <QList>

namespace nodetalk::net { class PeerManager; }

namespace nodetalk::ui {

class PeerListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        PeerIdRole = Qt::UserRole + 1,
        OnlineRole,
        TrustRole,
        IpRole,
        FingerprintRole,
        DisplayNameRole,
    };

    explicit PeerListModel(net::PeerManager& pm, QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& idx, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString peerIdAt(int row) const;
    int     rowOfPeer(const QString& peerId) const;

private slots:
    void onPeerAdded(const Peer& p);
    void onPeerUpdated(const Peer& p);

private:
    void rebuild();

    net::PeerManager& m_pm;
    QList<Peer>       m_rows;
};

} // namespace nodetalk::ui
