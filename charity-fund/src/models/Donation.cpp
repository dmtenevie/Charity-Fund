#include "Donation.h"

Donation::Donation() : id_(0), donorId_(0), projectId_(0), amount_(0.0) {}

Donation::Donation(int id, int donorId, int projectId, double amount,
                   const std::string& date, const std::string& paymentMethod,
                   const std::string& notes)
    : id_(id), donorId_(donorId), projectId_(projectId), amount_(amount),
      date_(date), paymentMethod_(paymentMethod), notes_(notes) {}

bool Donation::isValid() const {
    return amount_ > 0 && donorId_ > 0 && !date_.empty();
}
