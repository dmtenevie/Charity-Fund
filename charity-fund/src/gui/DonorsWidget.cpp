#include "DonorsWidget.h"
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
#include <QSplitter>

DonorsWidget::DonorsWidget(Repository* repo, QWidget* parent)
    : QWidget(parent), repository_(repo), selectedId_(-1) {
    setupUI();
    loadDonors();
}

void DonorsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(L::t("Донори (благодійники)"));
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    titleLayout->addWidget(titleLabel);

    toggleFormButton_ = new QPushButton(L::t("Показати форму"));
    toggleFormButton_->setObjectName("primaryButton");
    connect(toggleFormButton_, &QPushButton::clicked, this, &DonorsWidget::toggleFormPanel);
    titleLayout->addWidget(toggleFormButton_);
    titleLayout->addStretch();
    mainLayout->addLayout(titleLayout);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(L::t("Пошук:")));
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText(L::t("Введіть ім'я, email або телефон..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &DonorsWidget::searchDonors);
    searchLayout->addWidget(searchEdit_);
    mainLayout->addLayout(searchLayout);

    table_ = new QTableWidget();
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({"ID", L::t("Ім'я"), "Email", L::t("Телефон"), L::t("Адреса")});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setColumnWidth(0, 50);
    table_->setColumnWidth(1, 200);
    table_->setColumnWidth(2, 250);
    table_->setColumnWidth(3, 150);
    table_->setSortingEnabled(true);
    table_->sortByColumn(0, Qt::AscendingOrder);
    // A single click already highlights the row (SelectRows behavior), so it
    // must also arm Edit/Delete — otherwise they visibly look selectable but
    // fail with "select a record first". Double-click does the same plus
    // opens the form panel, so it stays the more visible reveal action.
    connect(table_, &QTableWidget::cellClicked, this, &DonorsWidget::selectDonorRow);
    connect(table_, &QTableWidget::cellDoubleClicked, this, &DonorsWidget::showDonorDetails);

    // A vertical splitter lets the user drag the divider to make the
    // add/edit form panel below taller or shorter relative to the table,
    // and the "Hide form" button can hide it completely.
    splitter_ = new QSplitter(Qt::Vertical);
    splitter_->addWidget(table_);

    formPanel_ = new QWidget();
    QVBoxLayout* formPanelLayout = new QVBoxLayout(formPanel_);
    formPanelLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* formGroup = new QGroupBox(L::t("Додати/Редагувати донора"));
    Theme::applyCardShadow(formGroup);
    QFormLayout* formLayout = new QFormLayout(formGroup);

    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText(L::t("Обов'язкове поле"));
    formLayout->addRow(L::t("Ім'я:"), nameEdit_);

    emailEdit_ = new QLineEdit();
    emailEdit_->setPlaceholderText("example@email.com");
    formLayout->addRow(L::t("Email:"), emailEdit_);

    phoneEdit_ = new QLineEdit();
    phoneEdit_->setPlaceholderText("+380...");
    formLayout->addRow(L::t("Телефон:"), phoneEdit_);

    addressEdit_ = new QLineEdit();
    addressEdit_->setPlaceholderText(L::t("м. Київ, вул..."));
    formLayout->addRow(L::t("Адреса:"), addressEdit_);

    notesEdit_ = new QTextEdit();
    notesEdit_->setMaximumHeight(60);
    notesEdit_->setPlaceholderText(L::t("Додаткова інформація"));
    formLayout->addRow(L::t("Примітки:"), notesEdit_);

    formPanelLayout->addWidget(formGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    addButton_ = new QPushButton(L::t("Додати"));
    addButton_->setObjectName("primaryButton");
    connect(addButton_, &QPushButton::clicked, this, &DonorsWidget::addDonor);
    buttonLayout->addWidget(addButton_);

    editButton_ = new QPushButton(L::t("Редагувати"));
    connect(editButton_, &QPushButton::clicked, this, &DonorsWidget::editDonor);
    editButton_->setEnabled(false);
    buttonLayout->addWidget(editButton_);

    deleteButton_ = new QPushButton(L::t("Видалити"));
    deleteButton_->setObjectName("dangerButton");
    connect(deleteButton_, &QPushButton::clicked, this, &DonorsWidget::deleteDonor);
    deleteButton_->setEnabled(false);
    buttonLayout->addWidget(deleteButton_);

    QPushButton* clearButton = new QPushButton(L::t("Очистити"));
    connect(clearButton, &QPushButton::clicked, this, &DonorsWidget::clearForm);
    buttonLayout->addWidget(clearButton);

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
}

void DonorsWidget::toggleFormPanel() {
    bool nowVisible = !formPanel_->isVisible();
    formPanel_->setVisible(nowVisible);
    toggleFormButton_->setText(nowVisible ? L::t("Приховати форму") : L::t("Показати форму"));
}

void DonorsWidget::loadDonors() {
    // Reapply any active search query — otherwise a reload triggered after
    // add/edit/delete (which always calls loadDonors(), never searchDonors())
    // would silently drop back to the full unfiltered list while the search
    // box still shows the user's query.
    if (!searchEdit_->text().isEmpty()) {
        searchDonors();
        return;
    }

    // Sorting must be off while rows are inserted — QTableWidget re-sorts
    // after every setItem() when enabled, which shuffles rows out from
    // under this loop and scrambles which donor each row ends up holding.
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    std::vector<Donor> donors = repository_->getAllDonors();

    for (const auto& donor : donors) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new NumericTableWidgetItem(QString::number(donor.getId()), donor.getId()));
        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(donor.getName())));
        table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(donor.getEmail())));
        table_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(donor.getPhone())));
        table_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(donor.getAddress())));
    }
    table_->setSortingEnabled(true);
    table_->resizeColumnsToContents();
}

void DonorsWidget::addDonor() {
    std::string name = nameEdit_->text().toStdString();
    if (name.empty()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Будь ласка, введіть ім'я донора!"));
        return;
    }

    Donor donor;
    donor.setName(name);
    donor.setEmail(emailEdit_->text().toStdString());
    donor.setPhone(phoneEdit_->text().toStdString());
    donor.setAddress(addressEdit_->text().toStdString());
    donor.setNotes(notesEdit_->toPlainText().toStdString());

    if (!donor.isValid()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Некоректні дані! Перевірте email."));
        return;
    }

    if (repository_->addDonor(donor)) {
        QMessageBox::information(this, L::t("Успіх"), L::t("Донора додано успішно!"));
        clearForm();
        loadDonors();
    } else {
        QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося додати донора!") + "\n\n" + repository_->getLastError());
    }
}

void DonorsWidget::editDonor() {
    if (selectedId_ <= 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Спочатку виберіть донора з таблиці!"));
        return;
    }

    std::string name = nameEdit_->text().toStdString();
    if (name.empty()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Ім'я не може бути порожнім!"));
        return;
    }

    Donor donor;
    donor.setId(selectedId_);
    donor.setName(name);
    donor.setEmail(emailEdit_->text().toStdString());
    donor.setPhone(phoneEdit_->text().toStdString());
    donor.setAddress(addressEdit_->text().toStdString());
    donor.setNotes(notesEdit_->toPlainText().toStdString());

    if (!donor.isValid()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Некоректні дані! Перевірте email."));
        return;
    }

    if (repository_->updateDonor(donor)) {
        QMessageBox::information(this, L::t("Успіх"), L::t("Дані донора оновлено!"));
        clearForm();
        loadDonors();
    } else {
        QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося оновити донора!") + "\n\n" + repository_->getLastError());
    }
}

void DonorsWidget::deleteDonor() {
    if (selectedId_ <= 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Спочатку виберіть донора з таблиці!"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, L::t("Підтвердження"),
        L::t("Ви впевнені, що хочете видалити цього донора?\nВсі пов'язані пожертви також будуть видалені!"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (repository_->deleteDonor(selectedId_)) {
            QMessageBox::information(this, L::t("Успіх"), L::t("Донора видалено!"));
            clearForm();
            loadDonors();
        } else {
            QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося видалити донора!") + "\n\n" + repository_->getLastError());
        }
    }
}

void DonorsWidget::searchDonors() {
    QString query = searchEdit_->text();
    if (query.isEmpty()) {
        loadDonors();
        return;
    }

    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    std::vector<Donor> donors = repository_->searchDonors(query);

    bool selectionStillVisible = false;
    for (const auto& donor : donors) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new NumericTableWidgetItem(QString::number(donor.getId()), donor.getId()));
        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(donor.getName())));
        table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(donor.getEmail())));
        table_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(donor.getPhone())));
        table_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(donor.getAddress())));
        if (donor.getId() == selectedId_) selectionStillVisible = true;
    }
    table_->setSortingEnabled(true);
    table_->resizeColumnsToContents();

    // The selected donor (and the Edit/Delete buttons targeting it) must not
    // survive a filter that hides it — otherwise Edit/Delete keep acting on
    // a record no longer shown in the table.
    if (selectedId_ > 0 && !selectionStillVisible) {
        clearForm();
    }
}

void DonorsWidget::selectDonorRow(int row, int column) {
    Q_UNUSED(column);
    int id = table_->item(row, 0)->text().toInt();
    auto donorOpt = repository_->getDonorById(id);

    if (donorOpt.has_value()) {
        fillForm(donorOpt.value());
        editButton_->setEnabled(true);
        deleteButton_->setEnabled(true);
    }
}

void DonorsWidget::showDonorDetails(int row, int column) {
    selectDonorRow(row, column);
    if (!formPanel_->isVisible()) toggleFormPanel();
}

void DonorsWidget::clearForm() {
    nameEdit_->clear();
    emailEdit_->clear();
    phoneEdit_->clear();
    addressEdit_->clear();
    notesEdit_->clear();
    selectedId_ = -1;
    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
}

void DonorsWidget::fillForm(const Donor& donor) {
    selectedId_ = donor.getId();
    nameEdit_->setText(QString::fromStdString(donor.getName()));
    emailEdit_->setText(QString::fromStdString(donor.getEmail()));
    phoneEdit_->setText(QString::fromStdString(donor.getPhone()));
    addressEdit_->setText(QString::fromStdString(donor.getAddress()));
    notesEdit_->setText(QString::fromStdString(donor.getNotes()));
}

void DonorsWidget::refresh() {
    loadDonors();
}
