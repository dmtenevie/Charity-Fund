#ifndef BENEFICIARIESWIDGET_H
#define BENEFICIARIESWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSplitter>
#include "../database/Repository.h"

class BeneficiariesWidget : public QWidget {
    Q_OBJECT

public:
    explicit BeneficiariesWidget(Repository* repo, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void addBeneficiary();
    void editBeneficiary();
    void deleteBeneficiary();
    void selectBeneficiaryRow(int row, int column);
    void showBeneficiaryDetails(int row, int column);
    void toggleFormPanel();
    void filterBeneficiaries();

private:
    void setupUI();
    void loadBeneficiaries();
    void loadProjects();
    void clearForm();
    void fillForm(const Beneficiary& beneficiary);

    Repository* repository_;
    QTableWidget* table_;
    QSplitter* splitter_;
    QWidget* formPanel_;
    QPushButton* toggleFormButton_;
    QLineEdit* searchEdit_;
    QLineEdit* nameEdit_;
    QLineEdit* contactEdit_;
    QComboBox* projectCombo_;
    QLineEdit* assistanceTypeEdit_;
    QTextEdit* descriptionEdit_;
    QPushButton* addButton_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;

    int selectedId_;
};

#endif // BENEFICIARIESWIDGET_H
