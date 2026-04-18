#pragma once

#include <QDialog>
#include <QHostAddress>

class QLineEdit;
class QSpinBox;

namespace nodetalk::ui {

class ManualAddPeerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ManualAddPeerDialog(QWidget* parent = nullptr);

    QHostAddress address() const;
    quint16      port()    const;

private:
    QLineEdit* m_ip   = nullptr;
    QSpinBox*  m_port = nullptr;
};

} // namespace nodetalk::ui
