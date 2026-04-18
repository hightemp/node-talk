#include "MessageInput.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>

namespace nodetalk::ui {

MessageInput::MessageInput(QWidget* parent) : QWidget(parent)
{
    setAcceptDrops(true);

    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);

    m_attach = new QToolButton(this);
    m_attach->setIcon(QIcon(QStringLiteral(":/resources/icons/attach.svg")));
    m_attach->setToolTip(tr("Attach file…"));
    m_attach->setAutoRaise(true);

    m_edit = new QPlainTextEdit(this);
    m_edit->setPlaceholderText(tr("Type a message…"));
    m_edit->setMaximumBlockCount(1000);
    m_edit->setFixedHeight(60);
    m_edit->installEventFilter(this);

    m_send = new QPushButton(tr("Send"), this);
    m_send->setIcon(QIcon(QStringLiteral(":/resources/icons/send.svg")));
    m_send->setDefault(true);
    // Stretch Send/Attach to the full height of the input row.
    m_send->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_send->setMinimumHeight(m_edit->height());
    m_attach->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_attach->setMinimumHeight(m_edit->height());

    lay->addWidget(m_attach);
    lay->addWidget(m_edit, 1);
    lay->addWidget(m_send);

    connect(m_send, &QPushButton::clicked, this, [this] {
        const QString text = m_edit->toPlainText().trimmed();
        if (!text.isEmpty()) {
            emit sendRequested(text);
            m_edit->clear();
            emit typingStateChanged(false);
        }
    });
    connect(m_attach, &QToolButton::clicked, this, &MessageInput::attachRequested);

    auto* typingDebounce = new QTimer(this);
    typingDebounce->setSingleShot(true);
    typingDebounce->setInterval(1500);
    connect(typingDebounce, &QTimer::timeout, this, [this] { emit typingStateChanged(false); });
    connect(m_edit, &QPlainTextEdit::textChanged, this, [this, typingDebounce] {
        emit typingStateChanged(!m_edit->toPlainText().isEmpty());
        typingDebounce->start();
    });
}

void MessageInput::setEnabledForSend(bool enabled)
{
    m_edit->setEnabled(enabled);
    m_send->setEnabled(enabled);
    m_attach->setEnabled(enabled);
    m_edit->setPlaceholderText(enabled ? tr("Type a message…") : tr("Peer is offline"));
}

void MessageInput::clear() { m_edit->clear(); }

bool MessageInput::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == m_edit && e->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(e);
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
            && !(ke->modifiers() & Qt::ShiftModifier)) {
            m_send->click();
            return true;
        }
    }
    return QWidget::eventFilter(obj, e);
}

void MessageInput::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MessageInput::dropEvent(QDropEvent* e)
{
    QStringList paths;
    for (const auto& url : e->mimeData()->urls()) {
        if (url.isLocalFile()) paths << url.toLocalFile();
    }
    if (!paths.isEmpty()) emit filesDropped(paths);
}

} // namespace nodetalk::ui
