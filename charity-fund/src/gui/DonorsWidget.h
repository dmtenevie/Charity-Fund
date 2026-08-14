#ifndef DONORSWIDGET_H
#define DONORSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSplitter>
#include "../database/Repository.h"

class DonorsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DonorsWidget(Repository* repo, QWidget* parent = nullptr);

public slots:
    void refresh();

private slots:
    void addDonor();
    void editDonor();
    void deleteDonor();
    void searchDonors();
    void selectDonorRow(int row, int column);
    void showDonorDetails(int row, int column);
    void toggleFormPanel();

private:
    void setupUI();
    void loadDonors();
    void clearForm();
    void fillForm(const Donor& donor);

    Repository* repository_;
    QTableWidget* table_;
    QSplitter* splitter_;
    QWidget* formPanel_;
    QPushButton* toggleFormButton_;
    QLineEdit* nameEdit_;
    QLineEdit* emailEdit_;
    QLineEdit* phoneEdit_;
    QLineEdit* addressEdit_;
    QTextEdit* notesEdit_;
    QLineEdit* searchEdit_;
    QPushButton* addButton_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;

    int selectedId_;
};

#endif // DONORSWIDGET_H
