#ifndef PROJECT_H
#define PROJECT_H

#include <string>

class Project {
public:
    Project();
    Project(int id, const std::string& name, const std::string& description,
            double goalAmount, double currentAmount,
            const std::string& startDate, const std::string& endDate,
            const std::string& status);

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    std::string getDescription() const { return description_; }
    void setDescription(const std::string& desc) { description_ = desc; }

    double getGoalAmount() const { return goalAmount_; }
    void setGoalAmount(double amount) { goalAmount_ = amount; }

    double getCurrentAmount() const { return currentAmount_; }
    void setCurrentAmount(double amount) { currentAmount_ = amount; }

    std::string getStartDate() const { return startDate_; }
    void setStartDate(const std::string& date) { startDate_ = date; }

    std::string getEndDate() const { return endDate_; }
    void setEndDate(const std::string& date) { endDate_ = date; }

    std::string getStatus() const { return status_; }
    void setStatus(const std::string& status) { status_ = status; }

    double getProgressPercentage() const;
    bool isValid() const;

private:
    int id_;
    std::string name_;
    std::string description_;
    double goalAmount_;
    double currentAmount_;
    std::string startDate_;
    std::string endDate_;
    std::string status_;
};

#endif // PROJECT_H
