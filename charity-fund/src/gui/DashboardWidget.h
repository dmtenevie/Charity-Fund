#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVector>
#include "../database/Repository.h"

// "Огляд" — the landing dashboard: key metric cards plus a per-project
// progress list, all derived live from the Repository.
class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWidget(Repository* repository, QWidget* parent = nullptr);

public slots:
    void refresh();

private:
    Repository* repository_;

    QLabel* totalLabel_;
    QLabel* donationsLabel_;
    QLabel* donationsNoteLabel_;
    QLabel* donorsLabel_;
    QLabel* projectsLabel_;
    QLabel* projectsNoteLabel_;
    QWidget* projectList_;
    QVector<QProgressBar*> projectBars_;
    QWidget* recentList_;
};

#endif // DASHBOARDWIDGET_H
