#ifndef DONOR_H
#define DONOR_H

#include <string>
#include <ctime>

class Donor {
public:
    Donor();
    Donor(int id, const std::string& name, const std::string& email,
          const std::string& phone, const std::string& address,
          const std::string& notes = "");

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    std::string getEmail() const { return email_; }
    void setEmail(const std::string& email) { email_ = email; }

    std::string getPhone() const { return phone_; }
    void setPhone(const std::string& phone) { phone_ = phone; }

    std::string getAddress() const { return address_; }
    void setAddress(const std::string& address) { address_ = address; }

    std::string getNotes() const { return notes_; }
    void setNotes(const std::string& notes) { notes_ = notes; }

    std::string getRegistrationDate() const { return registrationDate_; }
    void setRegistrationDate(const std::string& date) { registrationDate_ = date; }

    bool isValid() const;

private:
    int id_;
    std::string name_;
    std::string email_;
    std::string phone_;
    std::string address_;
    std::string notes_;
    std::string registrationDate_;
};

#endif // DONOR_H
