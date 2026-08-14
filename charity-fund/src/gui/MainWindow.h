#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QStatusBar>
#include <QLabel>
#include <QAction>
#include <QPushButton>
#include "DonorsWidget.h"
#include "DonationsWidget.h"
#include "ProjectsWidget.h"
#include "BeneficiariesWidget.h"
#include "ReportsWidget.h"
#include "DashboardWidget.h"
#include "../database/Repository.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif
    void changeEvent(QEvent* event) override;

private slots:
    void refreshAll();
    void showAbout();
    void checkDatabaseConnection();
    void toggleTheme();
    void toggleLanguage();
    void navigate(int index);
    void toggleMaximizeRestore();

private:
    void setupUI();
    void setupActions();
    void updateStatusBar();
    void updateNavBadges();
    void retranslateUi();
    void applyAccentToWindow();

    QStackedWidget* stack_;
    DashboardWidget* dashboardWidget_;
    DonorsWidget* donorsWidget_;
    DonationsWidget* donationsWidget_;
    ProjectsWidget* projectsWidget_;
    BeneficiariesWidget* beneficiariesWidget_;
    ReportsWidget* reportsWidget_;

    // Left navigation buttons (Огляд + 5 sections), in stack order.
    QVector<QPushButton*> navButtons_;
    // Record-count badges next to Донори/Пожертви/Проекти/Бенефіціари (in that order).
    QVector<QLabel*> navBadges_;

    QWidget* headerWidget_;
    QPushButton* ukButton_;
    QPushButton* enButton_;
    QPushButton* lightButton_;
    QPushButton* darkButton_;
    QPushButton* minButton_;
    QPushButton* maxButton_;
    QPushButton* closeButton_;
    QPushButton* menuButton_;

    QLabel* statusLabel_;
    Repository* repository_;
};

#endif // MAINWINDOW_H
