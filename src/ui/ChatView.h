#pragma once

#include "model/Message.h"

#include <QListWidget>

namespace nodetalk { class Database; }
namespace nodetalk::net { class PeerManager; class FileTransferManager; }

namespace nodetalk::ui {

/// Scrollable conversation view for one peer.
/// History is paged from `MessageRepository`. Each item is rendered by
/// the default delegate but with right/left alignment and bubble look
/// achieved through item flags + alternating background.
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

private slots:
    void onContextMenu(const QPoint& pos);
    void copySelectionPlain();
    void copySelectionFull();

protected:
    void keyPressEvent(QKeyEvent* e) override;

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
