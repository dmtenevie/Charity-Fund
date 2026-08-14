#include "Project.h"
#include <QDate>

Project::Project()
    : id_(0), goalAmount_(0.0), currentAmount_(0.0), status_("active") {}

Project::Project(int id, const std::string& name, const std::string& description,
                 double goalAmount, double currentAmount,
                 const std::string& startDate, const std::string& endDate,
                 const std::string& status)
    : id_(id), name_(name), description_(description),
      goalAmount_(goalAmount), currentAmount_(currentAmount),
      startDate_(startDate), endDate_(endDate), status_(status) {}

double Project::getProgressPercentage() const {
    if (goalAmount_ <= 0) return 0.0;
    return (currentAmount_ / goalAmount_) * 100.0;
}

bool Project::isValid() const {
    if (name_.empty() || name_.length() > 255) return false;
    if (goalAmount_ < 0 || currentAmount_ < 0 || currentAmount_ > goalAmount_) return false;
    if (startDate_.empty()) return false;

    if (!endDate_.empty()) {
        QDate start = QDate::fromString(QString::fromStdString(startDate_), "yyyy-MM-dd");
        QDate end = QDate::fromString(QString::fromStdString(endDate_), "yyyy-MM-dd");
        if (start.isValid() && end.isValid() && end < start) return false;
    }

    return true;
}
