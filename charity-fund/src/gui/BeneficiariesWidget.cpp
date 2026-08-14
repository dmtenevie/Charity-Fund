#include "BeneficiariesWidget.h"
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

BeneficiariesWidget::BeneficiariesWidget(Repository* repo, QWidget* parent)
    : QWidget(parent), repository_(repo), selectedId_(-1) {
    setupUI();
    loadProjects();
    loadBeneficiaries();
}

void BeneficiariesWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(L::t("Бенефіціари (отримувачі допомоги)"));
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    titleLayout->addWidget(titleLabel);

    toggleFormButton_ = new QPushButton(L::t("Показати форму"));
    toggleFormButton_->setObjectName("primaryButton");
    connect(toggleFormButton_, &QPushButton::clicked, this, &BeneficiariesWidget::toggleFormPanel);
    titleLayout->addWidget(toggleFormButton_);
    titleLayout->addStretch();
    mainLayout->addLayout(titleLayout);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    searchLayout->addWidget(new QLabel(L::t("Пошук:")));
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText(L::t("Пошук за ім'ям, контактом чи типом допомоги..."));
    connect(searchEdit_, &QLineEdit::textChanged, this, &BeneficiariesWidget::filterBeneficiaries);
    searchLayout->addWidget(searchEdit_);
    mainLayout->addLayout(searchLayout);

    table_ = new QTableWidget();
    table_->setColumnCount(5);
    table_->setHorizontalHeaderLabels({"ID", L::t("Ім'я"), L::t("Контакт"), L::t("Проект"), L::t("Тип допомоги")});
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->setColumnWidth(0, 50);
    table_->setColumnWidth(1, 200);
    table_->setColumnWidth(2, 150);
    table_->setColumnWidth(3, 200);
    table_->setSortingEnabled(true);
    table_->sortByColumn(0, Qt::AscendingOrder);
    // A single click already highlights the row (SelectRows behavior), so it
    // must also arm Edit/Delete — otherwise they visibly look selectable but
    // fail with "select a record first". Double-click does the same plus
    // opens the form panel, so it stays the more visible reveal action.
    connect(table_, &QTableWidget::cellClicked, this, &BeneficiariesWidget::selectBeneficiaryRow);
    connect(table_, &QTableWidget::cellDoubleClicked, this, &BeneficiariesWidget::showBeneficiaryDetails);

    splitter_ = new QSplitter(Qt::Vertical);
    splitter_->addWidget(table_);

    formPanel_ = new QWidget();
    QVBoxLayout* formPanelLayout = new QVBoxLayout(formPanel_);
    formPanelLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox* formGroup = new QGroupBox(L::t("Додати/Редагувати бенефіціара"));
    Theme::applyCardShadow(formGroup);
    QFormLayout* formLayout = new QFormLayout(formGroup);

    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText(L::t("Обов'язкове поле"));
    formLayout->addRow(L::t("Ім'я:"), nameEdit_);

    contactEdit_ = new QLineEdit();
    contactEdit_->setPlaceholderText(L::t("Телефон або email"));
    formLayout->addRow(L::t("Контакт:"), contactEdit_);

    projectCombo_ = new QComboBox();
    projectCombo_->addItem(L::t("Без проекту"), 0);
    formLayout->addRow(L::t("Проект:"), projectCombo_);

    assistanceTypeEdit_ = new QLineEdit();
    assistanceTypeEdit_->setPlaceholderText(L::t("Напр. Фінансова підтримка"));
    formLayout->addRow(L::t("Тип допомоги:"), assistanceTypeEdit_);

    descriptionEdit_ = new QTextEdit();
    descriptionEdit_->setMaximumHeight(60);
    descriptionEdit_->setPlaceholderText(L::t("Додаткова інформація"));
    formLayout->addRow(L::t("Примітки:"), descriptionEdit_);

    formPanelLayout->addWidget(formGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    addButton_ = new QPushButton(L::t("Додати"));
    addButton_->setObjectName("primaryButton");
    connect(addButton_, &QPushButton::clicked, this, &BeneficiariesWidget::addBeneficiary);
    buttonLayout->addWidget(addButton_);

    editButton_ = new QPushButton(L::t("Редагувати"));
    connect(editButton_, &QPushButton::clicked, this, &BeneficiariesWidget::editBeneficiary);
    editButton_->setEnabled(false);
    buttonLayout->addWidget(editButton_);

    deleteButton_ = new QPushButton(L::t("Видалити"));
    deleteButton_->setObjectName("dangerButton");
    connect(deleteButton_, &QPushButton::clicked, this, &BeneficiariesWidget::deleteBeneficiary);
    deleteButton_->setEnabled(false);
    buttonLayout->addWidget(deleteButton_);

    QPushButton* clearButton = new QPushButton(L::t("Очистити"));
    connect(clearButton, &QPushButton::clicked, this, &BeneficiariesWidget::clearForm);
    buttonLayout->addWidget(clearButton);

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

void BeneficiariesWidget::toggleFormPanel() {
    bool nowVisible = !formPanel_->isVisible();
    formPanel_->setVisible(nowVisible);
    toggleFormButton_->setText(nowVisible ? L::t("Приховати форму") : L::t("Показати форму"));
}

void BeneficiariesWidget::loadProjects() {
    projectCombo_->clear();
    projectCombo_->addItem(L::t("Без проекту"), 0);
    std::vector<Project> projects = repository_->getAllProjects();
    for (const auto& project : projects) {
        projectCombo_->addItem(QString::fromStdString(project.getName()), project.getId());
    }
}

void BeneficiariesWidget::loadBeneficiaries() {
    // Sorting must be off while rows are inserted — see the same note in
    // DonorsWidget::loadDonors().
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    std::vector<Beneficiary> beneficiaries = repository_->getAllBeneficiaries();
    std::vector<Project> projects = repository_->getAllProjects();

    for (const auto& beneficiary : beneficiaries) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new NumericTableWidgetItem(QString::number(beneficiary.getId()), beneficiary.getId()));
        table_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(beneficiary.getName())));
        table_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(beneficiary.getContact())));

        QString projectName = L::t("Без проекту");
        for (const auto& project : projects) {
            if (project.getId() == beneficiary.getProjectId()) {
                projectName = QString::fromStdString(project.getName());
                break;
            }
        }
        table_->setItem(row, 3, new QTableWidgetItem(projectName));
        table_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(beneficiary.getAssistanceType())));
    }
    table_->setSortingEnabled(true);
    table_->resizeColumnsToContents();
    filterBeneficiaries();
}

void BeneficiariesWidget::filterBeneficiaries() {
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

void BeneficiariesWidget::addBeneficiary() {
    std::string name = nameEdit_->text().toStdString();
    if (name.empty()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Будь ласка, введіть ім'я бенефіціара!"));
        return;
    }

    Beneficiary beneficiary;
    beneficiary.setName(name);
    beneficiary.setContact(contactEdit_->text().toStdString());
    beneficiary.setProjectId(projectCombo_->currentData().toInt());
    beneficiary.setAssistanceType(assistanceTypeEdit_->text().toStdString());
    beneficiary.setDescription(descriptionEdit_->toPlainText().toStdString());

    if (!beneficiary.isValid()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Некоректні дані бенефіціара!"));
        return;
    }

    if (repository_->addBeneficiary(beneficiary)) {
        QMessageBox::information(this, L::t("Успіх"), L::t("Бенефіціара додано успішно!"));
        clearForm();
        loadBeneficiaries();
    } else {
        QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося додати бенефіціара!") + "\n\n" + repository_->getLastError());
    }
}

void BeneficiariesWidget::editBeneficiary() {
    if (selectedId_ <= 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Спочатку виберіть бенефіціара з таблиці!"));
        return;
    }

    std::string name = nameEdit_->text().toStdString();
    if (name.empty()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Ім'я не може бути порожнім!"));
        return;
    }

    Beneficiary beneficiary;
    beneficiary.setId(selectedId_);
    beneficiary.setName(name);
    beneficiary.setContact(contactEdit_->text().toStdString());
    beneficiary.setProjectId(projectCombo_->currentData().toInt());
    beneficiary.setAssistanceType(assistanceTypeEdit_->text().toStdString());
    beneficiary.setDescription(descriptionEdit_->toPlainText().toStdString());

    if (!beneficiary.isValid()) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Некоректні дані бенефіціара!"));
        return;
    }

    if (repository_->updateBeneficiary(beneficiary)) {
        QMessageBox::information(this, L::t("Успіх"), L::t("Дані бенефіціара оновлено!"));
        clearForm();
        loadBeneficiaries();
    } else {
        QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося оновити бенефіціара!") + "\n\n" + repository_->getLastError());
    }
}

void BeneficiariesWidget::deleteBeneficiary() {
    if (selectedId_ <= 0) {
        QMessageBox::warning(this, L::t("Помилка"), L::t("Спочатку виберіть бенефіціара з таблиці!"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, L::t("Підтвердження"),
        L::t("Ви впевнені, що хочете видалити цього бенефіціара?"), QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (repository_->deleteBeneficiary(selectedId_)) {
            QMessageBox::information(this, L::t("Успіх"), L::t("Бенефіціара видалено!"));
            clearForm();
            loadBeneficiaries();
        } else {
            QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося видалити бенефіціара!") + "\n\n" + repository_->getLastError());
        }
    }
}

void BeneficiariesWidget::selectBeneficiaryRow(int row, int column) {
    Q_UNUSED(column);
    int id = table_->item(row, 0)->text().toInt();
    auto beneficiaryOpt = repository_->getBeneficiaryById(id);

    if (beneficiaryOpt.has_value()) {
        fillForm(beneficiaryOpt.value());
        editButton_->setEnabled(true);
        deleteButton_->setEnabled(true);
    }
}

void BeneficiariesWidget::showBeneficiaryDetails(int row, int column) {
    selectBeneficiaryRow(row, column);
    if (!formPanel_->isVisible()) toggleFormPanel();
}

void BeneficiariesWidget::clearForm() {
    nameEdit_->clear();
    contactEdit_->clear();
    projectCombo_->setCurrentIndex(0);
    assistanceTypeEdit_->clear();
    descriptionEdit_->clear();
    selectedId_ = -1;
    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);
}

void BeneficiariesWidget::fillForm(const Beneficiary& beneficiary) {
    selectedId_ = beneficiary.getId();
    nameEdit_->setText(QString::fromStdString(beneficiary.getName()));
    contactEdit_->setText(QString::fromStdString(beneficiary.getContact()));

    int idx = projectCombo_->findData(beneficiary.getProjectId());
    projectCombo_->setCurrentIndex(idx >= 0 ? idx : 0);

    assistanceTypeEdit_->setText(QString::fromStdString(beneficiary.getAssistanceType()));
    descriptionEdit_->setText(QString::fromStdString(beneficiary.getDescription()));
}

void BeneficiariesWidget::refresh() {
    loadProjects();
    loadBeneficiaries();
}
