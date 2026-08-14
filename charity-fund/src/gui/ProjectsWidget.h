#ifndef PROJECTSWIDGET_H
#define PROJECTSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QDateEdit>
#include <QProgressBar>
#include <QDoubleSpinBox>
#include <QSplitter>
#include "../database/Repository.h"

class ProjectsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ProjectsWidget(Repository* repo, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void addProject();
    void deleteProject();
    void loadProjects();
    void showProjectProgress(int row, int column);
    void toggleFormPanel();
    void filterProjects();
    void reapplyProgressBars();

private:
    void setupUI();
    void setProgressBarForRow(int row, double progress);

    Repository* repository_;
    // Cached by loadProjects() so reapplyProgressBars() can look up each
    // row's progress after a re-sort without a DB round trip.
    std::vector<Project> lastProjects_;
    QTableWidget* table_;
    QSplitter* splitter_;
    QWidget* formPanel_;
    QPushButton* toggleFormButton_;
    QLineEdit* searchEdit_;
    QLineEdit* nameEdit_;
    QTextEdit* descEdit_;
    QDoubleSpinBox* goalEdit_;
    QDateEdit* startDateEdit_;
    QDateEdit* endDateEdit_;
};

#endif // PROJECTSWIDGET_H
