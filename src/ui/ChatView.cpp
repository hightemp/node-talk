#include "ChatView.h"

#include "db/Database.h"
#include "db/MessageRepository.h"
#include "net/FileTransferManager.h"
#include "net/PeerManager.h"

#include <QApplication>
#include <QBrush>
#include <QDateTime>
#include <QFont>
#include <QListWidgetItem>
#include <QPalette>
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

    // Pick bubble + text colours based on whether the active palette is dark
    // or light, so messages stay readable on either system theme.
    const QPalette pal = qApp->palette();
    const bool dark = pal.color(QPalette::Window).lightness() < 128;
    QColor bgOut, bgIn, fg;
    if (dark) {
        bgOut = QColor(0x2f, 0x4a, 0x6d);   // muted blue
        bgIn  = QColor(0x33, 0x36, 0x3d);   // soft graphite
        fg    = QColor(0xe6, 0xeb, 0xf2);   // off-white
    } else {
        bgOut = QColor(0xdc, 0xeb, 0xff);   // pastel blue
        bgIn  = QColor(0xf3, 0xf3, 0xf3);   // light gray
        fg    = QColor(0x14, 0x14, 0x14);   // near-black
    }
    item->setForeground(QBrush(fg));
    if (m.direction == MessageDirection::Outgoing) {
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        item->setBackground(QBrush(bgOut));
    } else {
        item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        item->setBackground(QBrush(bgIn));
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
            it->setText(t.section(QLatin1Char(' '), 0, -2)
                        + QLatin1Char(' ') + statusGlyph(s));
            break;
        }
    }
}

} // namespace nodetalk::ui
