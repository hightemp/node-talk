#include "PeerListModel.h"

#include "net/PeerManager.h"

#include <QFont>
#include <QIcon>

namespace nodetalk::ui {

PeerListModel::PeerListModel(net::PeerManager& pm, QObject* parent)
    : QAbstractListModel(parent), m_pm(pm)
{
    rebuild();
    connect(&m_pm, &net::PeerManager::peerAdded,    this, &PeerListModel::onPeerAdded);
    connect(&m_pm, &net::PeerManager::peerUpdated,  this, &PeerListModel::onPeerUpdated);
    connect(&m_pm, &net::PeerManager::peerOnlineChanged, this, [this] { rebuild(); });
}

int PeerListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant PeerListModel::data(const QModelIndex& idx, int role) const
{
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_rows.size()) return {};
    const Peer& p = m_rows.at(idx.row());
    switch (role) {
        case Qt::DisplayRole: {
            QString name = p.shownName();
            if (p.trust != TrustState::Trusted) name += QStringLiteral(" (untrusted)");
            return name;
        }
        case Qt::DecorationRole:
            return QIcon(p.online
                ? QStringLiteral(":/resources/icons/peer-online.svg")
                : QStringLiteral(":/resources/icons/peer-offline.svg"));
        case Qt::ToolTipRole:
            return QStringLiteral("%1\n%2:%3\nfp: %4")
                .arg(p.shownName(),
                     p.lastIp,
                     QString::number(p.lastPort),
                     p.fingerprint);
        case Qt::ForegroundRole:
            if (!p.online) return QVariant(QColor(150, 150, 150));
            return {};
        case Qt::FontRole: {
            QFont f;
            if (!p.online) f.setItalic(true);
            return f;
        }
        case PeerIdRole:        return p.id;
        case OnlineRole:        return p.online;
        case TrustRole:         return static_cast<int>(p.trust);
        case IpRole:            return p.lastIp;
        case FingerprintRole:   return p.fingerprint;
        case DisplayNameRole:   return p.shownName();
    }
    return {};
}

QHash<int, QByteArray> PeerListModel::roleNames() const
{
    auto base = QAbstractListModel::roleNames();
    base.insert(PeerIdRole,      "peerId");
    base.insert(OnlineRole,      "online");
    base.insert(TrustRole,       "trust");
    base.insert(IpRole,          "ip");
    base.insert(FingerprintRole, "fingerprint");
    base.insert(DisplayNameRole, "displayName");
    return base;
}

QString PeerListModel::peerIdAt(int row) const
{
    if (row < 0 || row >= m_rows.size()) return {};
    return m_rows.at(row).id;
}

int PeerListModel::rowOfPeer(const QString& peerId) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).id == peerId) return i;
    }
    return -1;
}

void PeerListModel::rebuild()
{
    beginResetModel();
    m_rows = m_pm.allPeers();
    endResetModel();
}

void PeerListModel::onPeerAdded(const Peer&) { rebuild(); }
void PeerListModel::onPeerUpdated(const Peer&) { rebuild(); }

} // namespace nodetalk::ui
