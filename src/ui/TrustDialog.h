#pragma once

#include "model/Peer.h"

#include <QDialog>

namespace nodetalk::ui {

class TrustDialog : public QDialog {
    Q_OBJECT
public:
    explicit TrustDialog(const Peer& p, QWidget* parent = nullptr);
};

} // namespace nodetalk::ui
