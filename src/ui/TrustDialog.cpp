#include "TrustDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace nodetalk::ui {

TrustDialog::TrustDialog(const Peer& p, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Trust new peer?"));

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(tr("A new peer wants to connect:"), this));

    auto* form = new QFormLayout;
    form->addRow(tr("Nickname:"),    new QLabel(p.nickname, this));
    form->addRow(tr("Address:"),     new QLabel(QStringLiteral("%1:%2").arg(p.lastIp).arg(p.lastPort), this));
    auto* fp = new QLabel(p.fingerprint, this);
    fp->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fp->setWordWrap(true);
    form->addRow(tr("Fingerprint:"), fp);
    lay->addLayout(form);

    auto* bb = new QDialogButtonBox(this);
    auto* trustBtn  = bb->addButton(tr("Trust"),  QDialogButtonBox::AcceptRole);
    auto* rejectBtn = bb->addButton(tr("Reject"), QDialogButtonBox::RejectRole);
    Q_UNUSED(trustBtn); Q_UNUSED(rejectBtn);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(bb);
}

} // namespace nodetalk::ui
