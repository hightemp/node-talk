#pragma once

#include <QWidget>

class QPlainTextEdit;
class QPushButton;
class QToolButton;

namespace nodetalk::ui {

/// Chat input row: multi-line edit + Send + Attach.
/// Emits `sendRequested(text)` on Enter (Shift+Enter inserts newline)
/// and `attachRequested()` when the paperclip is clicked or files are
/// dropped onto the input.
class MessageInput : public QWidget {
    Q_OBJECT
public:
    explicit MessageInput(QWidget* parent = nullptr);

    void setEnabledForSend(bool enabled);
    void clear();

signals:
    void sendRequested(const QString& text);
    void typingStateChanged(bool active);
    void attachRequested();
    void filesDropped(const QStringList& paths);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dropEvent(QDropEvent* e) override;

private:
    QPlainTextEdit* m_edit   = nullptr;
    QPushButton*    m_send   = nullptr;
    QToolButton*    m_attach = nullptr;
};

} // namespace nodetalk::ui
