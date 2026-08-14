#ifndef NUMERICTABLEWIDGETITEM_H
#define NUMERICTABLEWIDGETITEM_H

#include <QTableWidgetItem>

// A QTableWidgetItem that sorts by an underlying numeric value (stashed in
// Qt::UserRole) instead of comparing its displayed text. Plain
// QTableWidgetItem compares display strings lexically, so formatted values
// like "10 000,00" or row id "10" sort *before* "5 000,00"/"5" — this is
// what makes clicking a numeric column header in QTableWidget actually sort
// numerically while still showing the locale-formatted text.
class NumericTableWidgetItem : public QTableWidgetItem {
public:
    NumericTableWidgetItem(const QString& displayText, double sortValue)
        : QTableWidgetItem(displayText) {
        setData(Qt::UserRole, sortValue);
    }

    bool operator<(const QTableWidgetItem& other) const override {
        return data(Qt::UserRole).toDouble() < other.data(Qt::UserRole).toDouble();
    }
};

#endif // NUMERICTABLEWIDGETITEM_H
