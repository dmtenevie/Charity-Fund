#include "DonationsWidget.h"
#include "NumericTableWidgetItem.h"
#include "../Lang.h"
#include "../Theme.h"
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

DonationsWidget::DonationsWidget(Repository* repo, QWidget* parent)
    : QWidget(parent), repository_(repo) {
    setupUI();
    loadDonations();
}

void DonationsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(L::t("Облік пожертв"));
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    titleLayout->addWidget(titleLabel);

    toggleFormButton_ = new QPushButton(L::t("Показати форму"));
    toggleFormButton_->setObjectName("primaryButton");
    connect(toggleFormButton_, &QPushButton::clicked, this, &DonationsWidget::toggleFormPanel);
    titleLayout->addWidget(toggleFormButton_);
    titleLayout->addStretch();
    mainLayout->addLayout(titleLayout);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(L::t("Пошук:")));
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText(L::t("Пошук за донором, проектом чи приміткою..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &DonationsWidget::filterDonations);
    searchLayout->addWidget(searchEdit_);
    mainLayout->addLayout(searchLayout);

    table_ = new QTableWidget();
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels({"ID", L::t("Донор"), L::t("Проект"), L::t("Сума (грн)"), L::t("Дата"), L::t("Спосіб оплати"), L::t("Примітки")});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setColumnWidth(0, 50);
    table_->setColumnWidth(1, 150);
    table_->setColumnWidth(2, 150);
    table_->setColumnWidth(3, 120);
    table_->setColumnWidth(4, 100);
    table_->setColumnWidth(5, 120);
    table_->setSortingEnabled(true);
    table_->sortByColumn(4, Qt::DescendingOrder);

    splitter_ = new QSplitter(Qt::Vertical);
    splitter_->addWidget(table_);

    formPanel_ = new QWidget();
    QVBoxLayout* formPanelLayout = new QVBoxLayout(formPanel_);
    formPanelLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* formGroup = new QGroupBox(L::t("Зареєструвати пожертву"));
    Theme::applyCardShadow(formGroup);
    QFormLayout* formLayout = new QFormLayout(formGroup);

    donorCombo_ = new QComboBox();
    formLayout->addRow(L::t("Донор:"), donorCombo_);
    projectCombo_ = new QComboBox();
    projectCombo_->addItem(L::t("Без проекту"), 0);
    formLayout->addRow(L::t("Проект:"), projectCombo_);
    amountEdit_ = new QDoubleSpinBox();
    amountEdit_->setRange(0.01, 100000000.0);
    amountEdit_->setDecimals(2);
    amountEdit_->setSuffix(" " + L::t("грн"));
    amountEdit_->setValue(0.01);
    formLayout->addRow(L::t("Сума (грн):"), amountEdit_);

    dateEdit_ = new QDateEdit();
    dateEdit_->setDate(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    formLayout->addRow(L::t("Дата:"), dateEdit_);

    paymentCombo_ = new QComboBox();
    for (const char* code : {"cash", "bank_transfer", "card", "online"})
        paymentCombo_->addItem(L::paymentMethodLabel(code), QString(code));
    formLayout->addRow(L::t("Спосіб оплати:"), paymentCombo_);

    notesEdit_ = new QLineEdit();
    notesEdit_->setPlaceholderText(L::t("Додаткова інформація"));
    formLayout->addRow(L::t("Примітки:"), notesEdit_);

    formPanelLayout->addWidget(formGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* addButton = new QPushButton(L::t("Додати пожертву"));
    addButton->setObjectName("primaryButton");
    connect(addButton, &QPushButton::clicked, this, &DonationsWidget::addDonation);
    buttonLayout->addWidget(addButton);

    QPushButton* deleteButton = new QPushButton(L::t("Видалити"));
    deleteButton->setObjectName("dangerButton");
    connect(deleteButton, &QPushButton::clicked, this, &DonationsWidget::deleteDonation);
    buttonLayout->addWidget(deleteButton);

    QPushButton* refreshButton = new QPushButton(L::t("Оновити"));
    connect(refreshButton, &QPushButton::clicked, this, &DonationsWidget::loadDonations);
    buttonLayout->addWidget(refreshButton);

    buttonLayout->addStretch();
    formPanelLayout->addLayout(buttonLayout);

    splitter_->addWidget(formPanel_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 0);
    splitter_->setChildrenCollapsible(false);
    splitter_->setSizes({500, 190});
    mainLayout->addWidget(splitter_);

    // Land on the list, not the form — opening the form is an explicit
    // action (the button above), not the default view of the tab.
    formPanel_->setVisible(false);

    loadDonors();
    loadProjects();
}

void DonationsWidget::toggleFormPanel() {
    bool nowVisible = !formPanel_->isVisible();
    formPanel_->setVisible(nowVisible);
    toggleFormButton_->setText(nowVisible ? L::t("Приховати форму") : L::t("Показати форму"));
}

void DonationsWidget::loadDonors() {
    donorCombo_->clear();
    std::vector<Donor> donors = repository_->getAllDonors();
    for (const auto& donor : donors) {
        donorCombo_->addItem(QString::fromStdString(donor.getName()), donor.getId());
    }
}

void DonationsWidget::loadProjects() {
    projectCombo_->clear();
    projectCombo_->addItem(L::t("Без проекту"), 0);
    // Active projects only — there's no edit flow for an existing donation
    // (only add/delete), so unlike Beneficiaries there's no risk of hiding
    // an already-assigned project out from under an in-progress edit. This
    // just stops new donations from being logged against an already-closed
    // project.
    std::vector<Project> projects = repository_->getActiveProjects();
    for (const auto& project : projects) {
        projectCombo_->addItem(QString::fromStdString(project.getName()), project.getId());
    }
}

void DonationsWidget::loadDonations() {
    // Sorting must be off while rows are inserted — see the same note in
    // DonorsWidget::loadDonors().
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    std::vector<DonationView> donations = repository_->getAllDonationsWithNames();
    for (const auto& donation : donations) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new NumericTableWidgetItem(QString::number(donation.id), donation.id));
        table_->setItem(row, 1, new QTableWidgetItem(donation.donorName));
        table_->setItem(row, 2, new QTableWidgetItem(donation.projectName));
        table_->setItem(row, 3, new NumericTableWidgetItem(money(donation.amount), donation.amount));
        table_->setItem(row, 4, new QTableWidgetItem(donation.date));
        table_->setItem(row, 5, new QTableWidgetItem(L::paymentMethodLabel(donation.paymentMethod)));
        table_->setItem(row, 6, new QTableWidgetItem(donation.notes));
    }
    table_->setSortingEnabled(true);
    table_->resizeColumnsToContents();
    filterDonations();
}

void DonationsWidget::filterDonations() {
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

void DonationsWidget::addDonation() {
    if (donorCombo_->count() == 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Спочатку додайте хоча б одного донора!"));
        return;
    }

    double amount = amountEdit_->value();
    if (amount <= 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Введіть коректну суму (більше 0)!"));
        return;
    }

    Donation donation;
    donation.setDonorId(donorCombo_->currentData().toInt());
    donation.setProjectId(projectCombo_->currentData().toInt());
    donation.setAmount(amount);
    donation.setDate(dateEdit_->date().toString("yyyy-MM-dd").toStdString());
    donation.setPaymentMethod(paymentCombo_->currentData().toString().toStdString());
    donation.setNotes(notesEdit_->text().toStdString());

    if (!donation.isValid()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Некоректні дані пожертви!"));
        return;
    }

    if (repository_->addDonation(donation)) {
        QMessageBox::information(this, L::t("Успіх"), L::t("Пожертву зареєстровано!"));
        amountEdit_->setValue(0.01);
        notesEdit_->clear();
        dateEdit_->setDate(QDate::currentDate());
        loadDonations();
        loadProjects();
    } else {
        QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося додати пожертву!") + "\n\n" + repository_->getLastError());
    }
}

void DonationsWidget::deleteDonation() {
    int row = table_->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Виберіть пожертву для видалення!"));
        return;
    }

    int id = table_->item(row, 0)->text().toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(this, L::t("Підтвердження"),
        L::t("Видалити цю пожертву?"), QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (repository_->deleteDonation(id)) {
            QMessageBox::information(this, L::t("Успіх"), L::t("Пожертву видалено!"));
            loadDonations();
            loadProjects();
        } else {
            QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося видалити пожертву!") + "\n\n" + repository_->getLastError());
        }
    }
}

void DonationsWidget::refresh() {
    loadDonors();
    loadProjects();
    loadDonations();
}
