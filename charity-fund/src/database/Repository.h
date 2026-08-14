#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "Database.h"
#include "../models/Donor.h"
#include "../models/Donation.h"
#include "../models/Project.h"
#include "../models/Beneficiary.h"
#include <vector>
#include <optional>
#include <QString>

class QSqlQuery;

struct DonationView {
    int id;
    QString donorName;
    QString projectName;
    double amount;
    QString date;
    QString paymentMethod;
    QString notes;
};

struct DonorStat {
    int donorId;
    QString donorName;
    int totalDonations;
    double totalAmount;
    QString lastDonationDate;
};

class Repository {
public:
    Repository();

    std::vector<Donor> getAllDonors();
    std::optional<Donor> getDonorById(int id);
    bool addDonor(Donor& donor);
    bool updateDonor(const Donor& donor);
    bool deleteDonor(int id);
    std::vector<Donor> searchDonors(const QString& query);

    std::vector<Donation> getAllDonations();
    std::optional<Donation> getDonationById(int id);
    bool addDonation(Donation& donation);
    bool updateDonation(const Donation& donation);
    bool deleteDonation(int id);
    std::vector<Donation> getDonationsByDonor(int donorId);
    std::vector<Donation> getDonationsByProject(int projectId);

    std::vector<Project> getAllProjects();
    std::optional<Project> getProjectById(int id);
    bool addProject(Project& project);
    bool updateProject(const Project& project);
    bool deleteProject(int id);
    std::vector<Project> getActiveProjects();

    std::vector<Beneficiary> getAllBeneficiaries();
    std::optional<Beneficiary> getBeneficiaryById(int id);
    bool addBeneficiary(Beneficiary& beneficiary);
    bool updateBeneficiary(const Beneficiary& beneficiary);
    bool deleteBeneficiary(int id);

    double getTotalDonations();
    double getTotalDonationsByDonor(int donorId);
    int getDonationCountByDonor(int donorId);
    std::vector<DonorStat> getDonorStatistics();
    std::vector<DonationView> getAllDonationsWithNames();

    QString getLastError() const { return lastError_; }

private:
    void setLastError(const QSqlQuery& query);

    Database& db_;
    QString lastError_;
};

#endif // REPOSITORY_H
