#pragma once

#include "model/Message.h"

#include <QListWidget>

class QResizeEvent;

namespace nodetalk { class Database; }
namespace nodetalk::net { class PeerManager; class FileTransferManager; }

namespace nodetalk::ui {

/// Scrollable conversation view for one peer.
/// Each row is a real `QLabel` widget hosted inside a `QListWidgetItem`,
/// so users can select arbitrary text/words inside a message and copy
/// it with the standard shortcut. URLs are auto-linkified and open in
/// the system browser.
class ChatView : public QListWidget {
    Q_OBJECT
public:
    ChatView(Database& db, net::PeerManager& pm,
             net::FileTransferManager& xfer, QWidget* parent = nullptr);

    /// Switch the displayed conversation. Loads the most recent page.
    void setPeer(const QString& peerId);
    QString currentPeerId() const { return m_peerId; }

    /// Filters messages by `query`; pass empty to clear.
    void setSearchFilter(const QString& query);

public slots:
    void onMessageAppended(const Message& m);
    void onMessageStatusChanged(const QString& msgId, MessageStatus s);

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    void appendItem(const Message& m);
    QString statusGlyph(MessageStatus s) const;

    Database&                 m_db;
    net::PeerManager&         m_pm;
    net::FileTransferManager& m_xfer;
    QString                   m_peerId;
    QString                   m_filter;
};

} // namespace nodetalk::ui
