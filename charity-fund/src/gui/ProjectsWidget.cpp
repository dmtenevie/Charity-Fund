#include "ProjectsWidget.h"
#include "NumericTableWidgetItem.h"
#include "../Theme.h"
#include "../Lang.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>
#include <QDate>
#include <QSplitter>
#include <QLocale>

namespace {
QString money(double v) {
    QLocale ua(QLocale::Ukrainian, QLocale::Ukraine);
    return ua.toString(v, 'f', 2);
}
} // namespace

ProjectsWidget::ProjectsWidget(Repository* repo, QWidget* parent)
    : QWidget(parent), repository_(repo) {
    setupUI();
    loadProjects();
}

void ProjectsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(L::t("Благодійні проекти"));
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    titleLayout->addWidget(titleLabel);

    toggleFormButton_ = new QPushButton(L::t("Показати форму"));
    toggleFormButton_->setObjectName("primaryButton");
    connect(toggleFormButton_, &QPushButton::clicked, this, &ProjectsWidget::toggleFormPanel);
    titleLayout->addWidget(toggleFormButton_);
    titleLayout->addStretch();
    mainLayout->addLayout(titleLayout);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(L::t("Пошук:")));
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText(L::t("Пошук за назвою, описом чи статусом..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &ProjectsWidget::filterProjects);
    searchLayout->addWidget(searchEdit_);
    mainLayout->addLayout(searchLayout);

    table_ = new QTableWidget();
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels({"ID", L::t("Назва"), L::t("Ціль (грн)"), L::t("Зібрано (грн)"),
                                       L::t("Прогрес"), L::t("Початок"), L::t("Статус")});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setColumnWidth(0, 50);
    table_->setColumnWidth(1, 250);
    table_->setColumnWidth(2, 120);
    table_->setColumnWidth(3, 120);
    table_->setColumnWidth(4, 140);
    table_->setColumnWidth(5, 100);
    table_->setSortingEnabled(true);
    table_->sortByColumn(0, Qt::AscendingOrder);
    connect(table_, &QTableWidget::cellDoubleClicked, this, &ProjectsWidget::showProjectProgress);
    // Rebuild the progress-bar cell widgets after every re-sort — see
    // reapplyProgressBars() for why they'd otherwise go stale/mismatched.
    // Connected after setSortingEnabled(true) above, so Qt's own internal
    // sort-on-click connection (made when sorting was enabled) runs first
    // and this sees the already-reordered rows.
    connect(table_->horizontalHeader(), &QHeaderView::sortIndicatorChanged,
            this, &ProjectsWidget::reapplyProgressBars);

    splitter_ = new QSplitter(Qt::Vertical);
    splitter_->addWidget(table_);

    formPanel_ = new QWidget();
    QVBoxLayout* formPanelLayout = new QVBoxLayout(formPanel_);
    formPanelLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* formGroup = new QGroupBox(L::t("Створити новий проект"));
    Theme::applyCardShadow(formGroup);
    QFormLayout* formLayout = new QFormLayout(formGroup);

    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText(L::t("Назва проекту"));
    formLayout->addRow(L::t("Назва:"), nameEdit_);

    descEdit_ = new QTextEdit();
    descEdit_->setMaximumHeight(60);
    descEdit_->setPlaceholderText(L::t("Опис проекту"));
    formLayout->addRow(L::t("Опис:"), descEdit_);

    goalEdit_ = new QDoubleSpinBox();
    goalEdit_->setRange(0.01, 100000000.0);
    goalEdit_->setDecimals(2);
    goalEdit_->setSuffix(" " + L::t("грн"));
    goalEdit_->setValue(0.01);
    formLayout->addRow(L::t("Ціль (грн):"), goalEdit_);

    startDateEdit_ = new QDateEdit();
    startDateEdit_->setDate(QDate::currentDate());
    startDateEdit_->setCalendarPopup(true);
    formLayout->addRow(L::t("Дата початку:"), startDateEdit_);

    endDateEdit_ = new QDateEdit();
    endDateEdit_->setDate(QDate::currentDate().addMonths(6));
    endDateEdit_->setCalendarPopup(true);
    formLayout->addRow(L::t("Дата завершення:"), endDateEdit_);

    formPanelLayout->addWidget(formGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* addButton = new QPushButton(L::t("Створити проект"));
    addButton->setObjectName("primaryButton");
    connect(addButton, &QPushButton::clicked, this, &ProjectsWidget::addProject);
    buttonLayout->addWidget(addButton);

    QPushButton* deleteButton = new QPushButton(L::t("Видалити"));
    deleteButton->setObjectName("dangerButton");
    connect(deleteButton, &QPushButton::clicked, this, &ProjectsWidget::deleteProject);
    buttonLayout->addWidget(deleteButton);

    QPushButton* refreshButton = new QPushButton(L::t("Оновити"));
    connect(refreshButton, &QPushButton::clicked, this, &ProjectsWidget::loadProjects);
    buttonLayout->addWidget(refreshButton);

    buttonLayout->addStretch();
    formPanelLayout->addLayout(buttonLayout);

    splitter_->addWidget(formPanel_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 0);
    splitter_->setChildrenCollapsible(false);
    splitter_->setSizes({500, 220});
    mainLayout->addWidget(splitter_);

    // Land on the list, not the form — opening the form is an explicit
    // action (the button above), not the default view of the tab.
    formPanel_->setVisible(false);
}

void ProjectsWidget::toggleFormPanel() {
    bool nowVisible = !formPanel_->isVisible();
    formPanel_->setVisible(nowVisible);
    toggleFormButton_->setText(nowVisible ? L::t("Приховати форму") : L::t("Показати форму"));
}

void ProjectsWidget::setProgressBarForRow(int row, double progress) {
    // Progress-bar traffic light: needs-funding/halfway/complete must stay
    // visually distinct from each other and from the brand accent (also
    // green), so "needs funding" gets its own muted blue rather than reusing
    // the accent color.
    const QString lowProgress = Theme::isDark() ? "#7fa7d9" : "#4a7ba3";
    const QString green   = Theme::isDark() ? "#5cb894" : "#3f8f6f";
    const QString yellow  = Theme::isDark() ? "#d9a441" : "#b0812a";
    const QString track   = Theme::isDark() ? "#2a2e33" : "#eae6df";
    const QString meja    = Theme::isDark() ? "#2a2e33" : "#e4e0d9";
    const QString text    = Theme::isDark() ? "#eceae7" : "#191817";

    QProgressBar* progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(static_cast<int>(qBound(0.0, progress, 100.0)));
    progressBar->setFormat(QString::number(progress, 'f', 1) + "%");
    progressBar->setTextVisible(true);

    QString chunkColor = progress >= 100 ? green : (progress >= 50 ? yellow : lowProgress);
    progressBar->setStyleSheet(QString(
        "QProgressBar { border: 1px solid %2; border-radius: 4px; text-align: center; "
        "background-color: %3; color: %4; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 3px; }"
    ).arg(chunkColor, meja, track, text));

    table_->setCellWidget(row, 4, progressBar);
}

void ProjectsWidget::loadProjects() {
    // Sorting must be off while rows are inserted — see the same note in
    // DonorsWidget::loadDonors().
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    lastProjects_ = repository_->getAllProjects();

    for (const auto& project : lastProjects_) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new NumericTableWidgetItem(QString::number(project.getId()), project.getId()));
        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(project.getName())));
        table_->setItem(row, 2, new NumericTableWidgetItem(money(project.getGoalAmount()), project.getGoalAmount()));
        table_->setItem(row, 3, new NumericTableWidgetItem(money(project.getCurrentAmount()), project.getCurrentAmount()));
        table_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(project.getStartDate())));
        table_->setItem(row, 6, new QTableWidgetItem(L::projectStatusLabel(QString::fromStdString(project.getStatus()))));

        setProgressBarForRow(row, project.getProgressPercentage());
    }
    table_->setSortingEnabled(true);
    // setSortingEnabled(true) immediately reapplies the table's persisted
    // sort indicator (e.g. from a previous header click) to the freshly
    // inserted rows — silently, without emitting sortIndicatorChanged if the
    // indicator itself didn't change. That reorder is exactly what desyncs
    // the progress bars, so always rebuild them here too, not just on the
    // signal (which only fires for a *new* interactive re-sort).
    reapplyProgressBars();
    table_->resizeColumnsToContents();
    filterProjects();
}

void ProjectsWidget::reapplyProgressBars() {
    // QTableWidget reorders QTableWidgetItems on sort, but cell widgets set
    // via setCellWidget() (the progress bars) are not part of that model and
    // stay pinned to their old row — so after every re-sort, rebuild each
    // row's progress bar from its now-correct ID column instead of trusting
    // row position to still match the project it was built for.
    for (int row = 0; row < table_->rowCount(); ++row) {
        QTableWidgetItem* idItem = table_->item(row, 0);
        if (!idItem) continue;
        int id = idItem->text().toInt();
        for (const auto& project : lastProjects_) {
            if (project.getId() == id) {
                setProgressBarForRow(row, project.getProgressPercentage());
                break;
            }
        }
    }
}

void ProjectsWidget::filterProjects() {
    const QString query = searchEdit_->text().trimmed();
    for (int row = 0; row < table_->rowCount(); ++row) {
        bool match = query.isEmpty();
        for (int col = 1; !match && col < table_->columnCount(); ++col) {
            QTableWidgetItem* item = table_->item(row, col);
            if (item && item->text().contains(query, Qt::CaseInsensitive)) match = true;
        }
        table_->setRowHidden(row, !match);
    }
}

void ProjectsWidget::addProject() {
    std::string name = nameEdit_->text().toStdString();
    if (name.empty()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Введіть назву проекту!"));
        return;
    }

    double goalAmount = goalEdit_->value();
    if (goalAmount <= 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Введіть коректну цільову суму!"));
        return;
    }

    Project project;
    project.setName(name);
    project.setDescription(descEdit_->toPlainText().toStdString());
    project.setGoalAmount(goalAmount);
    project.setStartDate(startDateEdit_->date().toString("yyyy-MM-dd").toStdString());
    project.setEndDate(endDateEdit_->date().toString("yyyy-MM-dd").toStdString());
    project.setStatus("active");

    if (!project.isValid()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Некоректні дані проекту! Перевірте назву та дати (дата завершення не може бути раніше дати початку)."));
        return;
    }

    if (repository_->addProject(project)) {
        QMessageBox::information(this, L::t("Успіх"), L::t("Проект створено!"));
        nameEdit_->clear();
        descEdit_->clear();
        goalEdit_->setValue(0.01);
        loadProjects();
    } else {
        QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося створити проект!") + "\n\n" + repository_->getLastError());
    }
}

void ProjectsWidget::deleteProject() {
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Виберіть проект для видалення!"));
        return;
    }

    int id = table_->item(row, 0)->text().toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(this, L::t("Підтвердження"),
        L::t("Видалити цей проект?\nПожертви та бенефіціари будуть збережені, але прив'язка до проекту буде видалена."),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (repository_->deleteProject(id)) {
            QMessageBox::information(this, L::t("Успіх"), L::t("Проект видалено!"));
            loadProjects();
        } else {
            QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося видалити проект!") + "\n\n" + repository_->getLastError());
        }
    }
}

void ProjectsWidget::showProjectProgress(int row, int column) {
    Q_UNUSED(column);
    int id = table_->item(row, 0)->text().toInt();
    auto projectOpt = repository_->getProjectById(id);

    if (projectOpt.has_value()) {
        const Project& project = projectOpt.value();
        QString info = L::html(QString(
            "<h3>%1</h3>"
            "<p><b>Опис:</b> %2</p>"
            "<p><b>Ціль:</b> %3 грн</p>"
            "<p><b>Зібрано:</b> %4 грн</p>"
            "<p><b>Прогрес:</b> %5%</p>"
            "<p><b>Період:</b> %6 - %7</p>"
            "<p><b>Статус:</b> %8</p>"
        ).arg(QString::fromStdString(project.getName()))
         .arg(QString::fromStdString(project.getDescription()))
         .arg(money(project.getGoalAmount()))
         .arg(money(project.getCurrentAmount()))
         .arg(project.getProgressPercentage(), 0, 'f', 1)
         .arg(QString::fromStdString(project.getStartDate()))
         .arg(QString::fromStdString(project.getEndDate()))
         .arg(L::projectStatusLabel(QString::fromStdString(project.getStatus()))));

        QMessageBox::information(this, L::t("Деталі проекту"), info);
    }
}

void ProjectsWidget::refresh() {
    loadProjects();
}
