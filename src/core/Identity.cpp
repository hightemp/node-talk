#include "Identity.h"

#include "Settings.h"

#include <QByteArray>
#include <QRandomGenerator>
#include <QUuid>

namespace nodetalk {

Identity::Identity(Settings& s) : m_settings(s) {}

void Identity::ensure()
{
    m_peerId      = m_settings.peerId();
    m_fingerprint = m_settings.fingerprint();

    if (m_peerId.isEmpty()) {
        m_peerId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        m_settings.setPeerId(m_peerId);
    }
    if (m_fingerprint.isEmpty()) {
        QByteArray raw(32, Qt::Uninitialized);
        QRandomGenerator::system()->generate(
            reinterpret_cast<quint32*>(raw.data()),
            reinterpret_cast<quint32*>(raw.data() + raw.size()));
        m_fingerprint = QString::fromLatin1(raw.toHex());
        m_settings.setFingerprint(m_fingerprint);
    }
}

} // namespace nodetalk
