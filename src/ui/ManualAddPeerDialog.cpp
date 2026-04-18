#include "ManualAddPeerDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

namespace nodetalk::ui {

ManualAddPeerDialog::ManualAddPeerDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Add peer by IP"));

    auto* lay = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    m_ip = new QLineEdit(this);
    m_ip->setPlaceholderText(QStringLiteral("192.168.1.42"));
    form->addRow(tr("IP address"), m_ip);

    m_port = new QSpinBox(this);
    m_port->setRange(1, 65535);
    m_port->setValue(45823);
    form->addRow(tr("Port"), m_port);
    lay->addLayout(form);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(bb);
}

QHostAddress ManualAddPeerDialog::address() const { return QHostAddress(m_ip->text().trimmed()); }
quint16      ManualAddPeerDialog::port()    const { return static_cast<quint16>(m_port->value()); }

} // namespace nodetalk::ui
