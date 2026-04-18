#include "ChatView.h"

#include "db/Database.h"
#include "db/MessageRepository.h"
#include "net/FileTransferManager.h"
#include "net/PeerManager.h"

#include <QBrush>
#include <QDateTime>
#include <QFont>
#include <QListWidgetItem>
#include <algorithm>

namespace nodetalk::ui {

ChatView::ChatView(Database& db, net::PeerManager& pm,
                   net::FileTransferManager& xfer, QWidget* parent)
    : QListWidget(parent), m_db(db), m_pm(pm), m_xfer(xfer)
{
    setUniformItemSizes(false);
    setWordWrap(true);
    setSelectionMode(QAbstractItemView::NoSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(4);
    setStyleSheet(QStringLiteral("QListWidget::item { padding: 6px 8px; }"));

    connect(&m_pm, &net::PeerManager::messageAppended,
            this, &ChatView::onMessageAppended);
    connect(&m_pm, &net::PeerManager::messageStatusChanged,
            this, &ChatView::onMessageStatusChanged);
}

void ChatView::setPeer(const QString& peerId)
{
    m_peerId = peerId;
    clear();
    if (peerId.isEmpty()) return;

    MessageRepository repo(m_db);
    auto page = repo.page(peerId, 0, 200);
    std::reverse(page.begin(), page.end());
    for (const auto& m : page) appendItem(m);
    scrollToBottom();
}

void ChatView::setSearchFilter(const QString& query)
{
    m_filter = query;
    setPeer(m_peerId);
    if (query.isEmpty()) return;
    for (int i = 0; i < count(); ++i) {
        item(i)->setHidden(!item(i)->text().contains(query, Qt::CaseInsensitive));
    }
}

QString ChatView::statusGlyph(MessageStatus s) const
{
    switch (s) {
        case MessageStatus::Sending:  return QStringLiteral("⏳");
        case MessageStatus::Sent:     return QStringLiteral("✓");
        case MessageStatus::Delivered:return QStringLiteral("✓✓");
        case MessageStatus::Read:     return QStringLiteral("✓✓ (read)");
        case MessageStatus::Failed:   return QStringLiteral("✗");
    }
    return {};
}

void ChatView::appendItem(const Message& m)
{
    const QString time = QDateTime::fromSecsSinceEpoch(m.ts).toString(QStringLiteral("HH:mm"));
    QString text;
    if (m.direction == MessageDirection::Outgoing) {
        text = QStringLiteral("[%1] You: %2  %3").arg(time, m.body, statusGlyph(m.status));
    } else {
        text = QStringLiteral("[%1] %2").arg(time, m.body);
    }
    auto* item = new QListWidgetItem(text, this);
    item->setData(Qt::UserRole, m.id);
    if (m.direction == MessageDirection::Outgoing) {
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        item->setBackground(QBrush(QColor(220, 235, 255)));
    } else {
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item->setBackground(QBrush(QColor(245, 245, 245)));
    }
    if (m.kind == MessageKind::File) {
        QFont f = item->font();
        f.setItalic(true);
        item->setFont(f);
    }
}

void ChatView::onMessageAppended(const Message& m)
{
    if (m.peerId != m_peerId) return;
    appendItem(m);
    scrollToBottom();
}

void ChatView::onMessageStatusChanged(const QString& msgId, MessageStatus s)
{
    for (int i = count() - 1; i >= 0; --i) {
        auto* it = item(i);
        if (it->data(Qt::UserRole).toString() == msgId) {
            QString t = it->text();
            // crude: re-render the trailing glyph
            const int sep = t.lastIndexOf(QChar(' '));
            Q_UNUSED(sep);
            it->setText(t.section(QChar(' '), 0, -2) + QChar(' ') + statusGlyph(s));
            break;
        }
    }
}

} // namespace nodetalk::ui
