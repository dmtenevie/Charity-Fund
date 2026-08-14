#ifndef DONATION_H
#define DONATION_H

#include <string>

class Donation {
public:
    Donation();
    Donation(int id, int donorId, int projectId, double amount,
             const std::string& date, const std::string& paymentMethod,
             const std::string& notes = "");

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    int getDonorId() const { return donorId_; }
    void setDonorId(int id) { donorId_ = id; }

    int getProjectId() const { return projectId_; }
    void setProjectId(int id) { projectId_ = id; }

    double getAmount() const { return amount_; }
    void setAmount(double amount) { amount_ = amount; }

    std::string getDate() const { return date_; }
    void setDate(const std::string& date) { date_ = date; }

    std::string getPaymentMethod() const { return paymentMethod_; }
    void setPaymentMethod(const std::string& method) { paymentMethod_ = method; }

    std::string getNotes() const { return notes_; }
    void setNotes(const std::string& notes) { notes_ = notes; }

    bool isValid() const;

private:
    int id_;
    int donorId_;
    int projectId_;
    double amount_;
    std::string date_;
    std::string paymentMethod_;
    std::string notes_;
};

#endif // DONATION_H
