#ifndef DONATIONSWIDGET_H
#define DONATIONSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QSplitter>
#include "../database/Repository.h"

class DonationsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DonationsWidget(Repository* repo, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void addDonation();
    void deleteDonation();
    void loadDonations();
    void toggleFormPanel();
    void filterDonations();

private:
    void setupUI();
    void loadDonors();
    void loadProjects();
    Repository* repository_;
    QTableWidget* table_;
    QSplitter* splitter_;
    QWidget* formPanel_;
    QPushButton* toggleFormButton_;
    QLineEdit* searchEdit_;
    QComboBox* donorCombo_;
    QComboBox* projectCombo_;
    QDoubleSpinBox* amountEdit_;
    QDateEdit* dateEdit_;
    QComboBox* paymentCombo_;
    QLineEdit* notesEdit_;
};

#endif // DONATIONSWIDGET_H
