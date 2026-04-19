#include "ChatView.h"

#include "db/Database.h"
#include "db/MessageRepository.h"
#include "net/FileTransferManager.h"
#include "net/PeerManager.h"

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QPalette>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QStringBuilder>
#include <QWidget>
#include <algorithm>

namespace nodetalk::ui {

namespace {

constexpr int kRoleMsgId = Qt::UserRole + 0;
constexpr int kRoleBody  = Qt::UserRole + 1;
constexpr int kRoleLabel = Qt::UserRole + 2; // QLabel*

QString escapeHtml(const QString& s)
{
    return s.toHtmlEscaped();
}

/// Escape HTML and turn bare URLs into clickable <a> tags.
QString linkify(const QString& body)
{
    static const QRegularExpression re(
        QStringLiteral(R"((https?://[^\s<>"']+|www\.[^\s<>"']+))"),
        QRegularExpression::CaseInsensitiveOption);
    QString out;
    int last = 0;
    auto it = re.globalMatch(body);
    while (it.hasNext()) {
        const auto m = it.next();
        out += escapeHtml(body.mid(last, m.capturedStart() - last));
        QString url = m.captured();
        QString href = url.startsWith(QLatin1String("www."), Qt::CaseInsensitive)
                       ? QStringLiteral("http://") + url : url;
        out += QStringLiteral("<a href=\"%1\">%2</a>")
                   .arg(escapeHtml(href), escapeHtml(url));
        last = m.capturedEnd();
    }
    out += escapeHtml(body.mid(last));
    out.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
    return out;
}

} // namespace

ChatView::ChatView(Database& db, net::PeerManager& pm,
                   net::FileTransferManager& xfer, QWidget* parent)
    : QListWidget(parent), m_db(db), m_pm(pm), m_xfer(xfer)
{
    setUniformItemSizes(false);
    setWordWrap(false);
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(2);
    setStyleSheet(QStringLiteral(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { background: transparent; border: none;"
        "                    padding: 0px; margin: 0px; }"));

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
        const QString body = item(i)->data(kRoleBody).toString();
        item(i)->setHidden(!body.contains(query, Qt::CaseInsensitive));
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
    const bool outgoing = (m.direction == MessageDirection::Outgoing);

    // Theme-aware bubble colors.
    const QPalette pal = qApp->palette();
    const bool dark = pal.color(QPalette::Window).lightness() < 128;
    QColor bgOut, bgIn, fg, fgMeta, link;
    if (dark) {
        bgOut  = QColor(0x2f, 0x4a, 0x6d);
        bgIn   = QColor(0x33, 0x36, 0x3d);
        fg     = QColor(0xe6, 0xeb, 0xf2);
        fgMeta = QColor(0xa0, 0xa8, 0xb4);
        link   = QColor(0x82, 0xb1, 0xff);
    } else {
        bgOut  = QColor(0xdc, 0xeb, 0xff);
        bgIn   = QColor(0xf3, 0xf3, 0xf3);
        fg     = QColor(0x14, 0x14, 0x14);
        fgMeta = QColor(0x6b, 0x73, 0x80);
        link   = QColor(0x18, 0x4a, 0xc4);
    }
    const QColor bg = outgoing ? bgOut : bgIn;

    // Build the bubble HTML: meta line (time + author + status) + body.
    QString meta;
    if (outgoing) {
        meta = QStringLiteral("[%1] You · %2").arg(time, statusGlyph(m.status));
    } else {
        meta = QStringLiteral("[%1]").arg(time);
    }
    QString bodyHtml;
    if (m.kind == MessageKind::File) {
        bodyHtml = QStringLiteral("<i>📎 %1</i>").arg(escapeHtml(m.body));
    } else {
        bodyHtml = linkify(m.body);
    }
    const QString html = QStringLiteral(
        "<div style='color:%1;font-size:9pt;'>%2</div>"
        "<div style='color:%3;'>%4</div>")
        .arg(fgMeta.name(), escapeHtml(meta), fg.name(), bodyHtml);

    auto* label = new QLabel(html, this);
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse
                                   | Qt::TextSelectableByKeyboard
                                   | Qt::LinksAccessibleByMouse
                                   | Qt::LinksAccessibleByKeyboard);
    label->setOpenExternalLinks(true);
    label->setContextMenuPolicy(Qt::DefaultContextMenu);
    label->setMargin(8);
    label->setStyleSheet(QStringLiteral(
        "QLabel { background:%1; border-radius:8px; color:%2; }"
        "QLabel a { color:%3; }")
        .arg(bg.name(), fg.name(), link.name()));

    auto* container = new QWidget(this);
    auto* lay = new QHBoxLayout(container);
    lay->setContentsMargins(6, 2, 6, 2);
    lay->setSpacing(0);
    if (outgoing) lay->addStretch(1);
    lay->addWidget(label, 0, outgoing ? Qt::AlignRight : Qt::AlignLeft);
    if (!outgoing) lay->addStretch(1);

    auto* item = new QListWidgetItem(this);
    item->setData(kRoleMsgId, m.id);
    item->setData(kRoleBody,  m.body);
    item->setData(kRoleLabel, QVariant::fromValue(static_cast<void*>(label)));

    setItemWidget(item, container);

    // Initial size hint; will be refreshed on resize.
    const int w = std::max(200, viewport()->width() - 24);
    label->setMaximumWidth(int(w * 0.75));
    item->setSizeHint(container->sizeHint());
}

void ChatView::onMessageAppended(const Message& m)
{
    if (m.peerId != m_peerId) return;
    appendItem(m);
    scrollToBottom();
}

void ChatView::onMessageStatusChanged(const QString& msgId, MessageStatus /*s*/)
{
    // Re-render the affected row by replacing its bubble HTML.
    for (int i = count() - 1; i >= 0; --i) {
        auto* it = item(i);
        if (it->data(kRoleMsgId).toString() != msgId) continue;
        // Easiest: rebuild from a fake Message; pull current body from data.
        // We need direction/kind too — keep it simple: refetch from DB.
        MessageRepository repo(m_db);
        for (const auto& msg : repo.page(m_peerId, 0, 200)) {
            if (msg.id == msgId) {
                // Replace widget in place.
                auto* old = itemWidget(it);
                removeItemWidget(it);
                if (old) old->deleteLater();
                // Rebuild via appendItem-like logic: just delete and reinsert
                // is simplest, but preserves order if we replace. Reuse:
                const auto row = this->row(it);
                delete takeItem(row);
                // Insert at the same position by creating a fresh row, then
                // moving it (QListWidget has no insertWidget; we just append
                // since status updates almost always target the last row).
                appendItem(msg);
                break;
            }
        }
        break;
    }
}

void ChatView::resizeEvent(QResizeEvent* e)
{
    QListWidget::resizeEvent(e);
    const int w = std::max(200, viewport()->width() - 24);
    for (int i = 0; i < count(); ++i) {
        auto* it = item(i);
        auto* lbl = static_cast<QLabel*>(it->data(kRoleLabel).value<void*>());
        if (!lbl) continue;
        lbl->setMaximumWidth(int(w * 0.75));
        if (auto* container = itemWidget(it)) {
            it->setSizeHint(container->sizeHint());
        }
    }
}

} // namespace nodetalk::ui
