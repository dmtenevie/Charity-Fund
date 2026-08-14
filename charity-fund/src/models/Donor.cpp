#include "Donor.h"
#include <regex>

Donor::Donor() : id_(0) {}

Donor::Donor(int id, const std::string& name, const std::string& email,
             const std::string& phone, const std::string& address,
             const std::string& notes)
    : id_(id), name_(name), email_(email), phone_(phone),
      address_(address), notes_(notes) {}

bool Donor::isValid() const {
    if (name_.empty() || name_.length() < 2 || name_.length() > 255) {
        return false;
    }

    if (!email_.empty()) {
        if (email_.length() > 255) {
            return false;
        }
        std::regex emailPattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        if (!std::regex_match(email_, emailPattern)) {
            return false;
        }
    }

    if (phone_.length() > 50) {
        return false;
    }

    return true;
}
