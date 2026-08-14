#ifndef BENEFICIARY_H
#define BENEFICIARY_H

#include <string>

class Beneficiary {
public:
    Beneficiary();
    Beneficiary(int id, const std::string& name, const std::string& contact,
                int projectId, const std::string& description,
                const std::string& assistanceType);

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    std::string getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    std::string getContact() const { return contact_; }
    void setContact(const std::string& contact) { contact_ = contact; }

    int getProjectId() const { return projectId_; }
    void setProjectId(int id) { projectId_ = id; }

    std::string getDescription() const { return description_; }
    void setDescription(const std::string& desc) { description_ = desc; }

    std::string getAssistanceType() const { return assistanceType_; }
    void setAssistanceType(const std::string& type) { assistanceType_ = type; }

    bool isValid() const;

private:
    int id_;
    std::string name_;
    std::string contact_;
    int projectId_;
    std::string description_;
    std::string assistanceType_;
};

#endif // BENEFICIARY_H
