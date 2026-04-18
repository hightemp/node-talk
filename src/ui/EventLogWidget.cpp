#include "EventLogWidget.h"

#include "core/EventLog.h"

#include <QDateTime>
#include <QHeaderView>

namespace nodetalk::ui {

EventLogWidget::EventLogWidget(EventLog& log, QWidget* parent)
    : QTreeWidget(parent), m_log(log)
{
    setColumnCount(4);
    setHeaderLabels({tr("Time"), tr("Level"), tr("Kind"), tr("Text")});
    header()->setStretchLastSection(true);
    setRootIsDecorated(false);
    setUniformRowHeights(true);

    for (const auto& e : m_log.repo().recent(200)) addItem(e);
    connect(&m_log, &EventLog::appended, this, &EventLogWidget::onAppended);
}

void EventLogWidget::addItem(const Event& e)
{
    auto* item = new QTreeWidgetItem;
    item->setText(0, QDateTime::fromSecsSinceEpoch(e.ts).toString(Qt::ISODate));
    static const char* lvl[] = {"INFO", "WARN", "ERROR"};
    item->setText(1, QString::fromLatin1(lvl[std::clamp(e.level, 0, 2)]));
    item->setText(2, e.kind);
    item->setText(3, e.text);
    insertTopLevelItem(0, item);
}

void EventLogWidget::onAppended(const Event& e) { addItem(e); }

} // namespace nodetalk::ui
