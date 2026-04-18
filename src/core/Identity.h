#pragma once

#include <QString>

namespace nodetalk {

class Settings;

/// Stable per-machine identity. The pair (peerId, fingerprint) is the
/// durable identity used by the trust model and is independent of IP
/// address.
class Identity {
public:
    explicit Identity(Settings& s);

    /// Loads existing identity or generates a new one and persists it.
    void ensure();

    QString peerId() const      { return m_peerId; }
    QString fingerprint() const { return m_fingerprint; }

private:
    Settings& m_settings;
    QString   m_peerId;
    QString   m_fingerprint;
};

} // namespace nodetalk
