#pragma once

#include <QHostAddress>
#include <QString>

namespace nodetalk {

enum class TrustState : int {
    Unknown = 0,
    Trusted = 1,
    Blocked = 2,
};

struct Peer {
    QString    id;             // peer_id (UUID)
    QString    fingerprint;
    QString    nickname;       // remote-advertised
    QString    displayName;    // local override (optional)
    TrustState trust    = TrustState::Unknown;
    QString    lastIp;
    quint16    lastPort = 0;
    qint64     lastSeen = 0;   // unix seconds
    qint64     createdAt = 0;
    bool       online   = false; // derived, not stored

    QString shownName() const { return displayName.isEmpty() ? nickname : displayName; }
};

} // namespace nodetalk
