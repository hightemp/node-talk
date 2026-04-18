#pragma once

#include "db/EventRepository.h"

#include <QTreeWidget>

namespace nodetalk { class EventLog; }

namespace nodetalk::ui {

class EventLogWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit EventLogWidget(EventLog& log, QWidget* parent = nullptr);

private slots:
    void onAppended(const Event& e);

private:
    void addItem(const Event& e);
    EventLog& m_log;
};

} // namespace nodetalk::ui
