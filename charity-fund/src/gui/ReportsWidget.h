#ifndef REPORTSWIDGET_H
#define REPORTSWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QChartView>
#include "../database/Repository.h"

class ReportsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReportsWidget(Repository* repo, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void generateReport();
    void exportReport();

private:
    void setupUI();
    QString generateDonorStatistics();
    QString generateProjectStatistics();
    QString generateFinancialReport();
    void updateCharts();

    Repository* repository_;
    QTextEdit* reportView_;
    QComboBox* reportTypeCombo_;
    QChartView* projectsChartView_;
    QChartView* donorsChartView_;
};

#endif // REPORTSWIDGET_H
