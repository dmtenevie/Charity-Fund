#include "Beneficiary.h"

Beneficiary::Beneficiary() : id_(0), projectId_(0) {}

Beneficiary::Beneficiary(int id, const std::string& name, const std::string& contact,
                         int projectId, const std::string& description,
                         const std::string& assistanceType)
    : id_(id), name_(name), contact_(contact), projectId_(projectId),
      description_(description), assistanceType_(assistanceType) {}

bool Beneficiary::isValid() const {
    if (name_.empty() || name_.length() < 2 || name_.length() > 255) return false;
    if (contact_.length() > 255) return false;
    if (assistanceType_.length() > 100) return false;
    return true;
}
