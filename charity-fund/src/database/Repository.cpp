#include "Repository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace {
// donors.email is UNIQUE but optional in the UI/model (Donor::isValid()
// only checks its format when non-empty). Binding "" would make every
// donor left without an email collide on that empty string once a second
// one is added — NULL is exempt from the uniqueness check, "" is not.
QVariant emailOrNull(const std::string& email) {
    if (email.empty()) return QVariant(QMetaType(QMetaType::QString));
    return QString::fromStdString(email);
}
} // namespace

Repository::Repository() : db_(Database::getInstance()) {}

void Repository::setLastError(const QSqlQuery& query) {
    lastError_ = query.lastError().text();
}

std::vector<Donor> Repository::getAllDonors() {
    std::vector<Donor> donors;
    QSqlQuery query(db_.getConnection());
    
    if (!query.exec("SELECT id, name, email, phone, address, notes, registration_date FROM donors ORDER BY id")) {
        setLastError(query);
        return donors;
    }

    while (query.next()) {
        Donor donor;
        donor.setId(query.value(0).toInt());
        donor.setName(query.value(1).toString().toStdString());
        donor.setEmail(query.value(2).toString().toStdString());
        donor.setPhone(query.value(3).toString().toStdString());
        donor.setAddress(query.value(4).toString().toStdString());
        donor.setNotes(query.value(5).toString().toStdString());
        donor.setRegistrationDate(query.value(6).toString().toStdString());
        donors.push_back(donor);
    }
    return donors;
}

std::optional<Donor> Repository::getDonorById(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("SELECT id, name, email, phone, address, notes, registration_date FROM donors WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }
    
    Donor donor;
    donor.setId(query.value(0).toInt());
    donor.setName(query.value(1).toString().toStdString());
    donor.setEmail(query.value(2).toString().toStdString());
    donor.setPhone(query.value(3).toString().toStdString());
    donor.setAddress(query.value(4).toString().toStdString());
    donor.setNotes(query.value(5).toString().toStdString());
    donor.setRegistrationDate(query.value(6).toString().toStdString());
    return donor;
}

bool Repository::addDonor(Donor& donor) {
    QSqlQuery query(db_.getConnection());
    query.prepare("INSERT INTO donors (name, email, phone, address, notes) "
                  "VALUES (?, ?, ?, ?, ?) RETURNING id");
    query.addBindValue(QString::fromStdString(donor.getName()));
    query.addBindValue(emailOrNull(donor.getEmail()));
    query.addBindValue(QString::fromStdString(donor.getPhone()));
    query.addBindValue(QString::fromStdString(donor.getAddress()));
    query.addBindValue(QString::fromStdString(donor.getNotes()));

    if (query.exec() && query.next()) {
        donor.setId(query.value(0).toInt());
        return true;
    }
    setLastError(query);
    return false;
}

bool Repository::updateDonor(const Donor& donor) {
    QSqlQuery query(db_.getConnection());
    query.prepare("UPDATE donors SET name = ?, email = ?, phone = ?, address = ?, notes = ? WHERE id = ?");
    query.addBindValue(QString::fromStdString(donor.getName()));
    query.addBindValue(emailOrNull(donor.getEmail()));
    query.addBindValue(QString::fromStdString(donor.getPhone()));
    query.addBindValue(QString::fromStdString(donor.getAddress()));
    query.addBindValue(QString::fromStdString(donor.getNotes()));
    query.addBindValue(donor.getId());

    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}

bool Repository::deleteDonor(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("DELETE FROM donors WHERE id = ?");
    query.addBindValue(id);
    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}

std::vector<Donor> Repository::searchDonors(const QString& searchQuery) {
    std::vector<Donor> donors;
    QSqlQuery query(db_.getConnection());
    query.prepare("SELECT id, name, email, phone, address, notes, registration_date FROM donors "
                  "WHERE name ILIKE ? OR email ILIKE ? OR phone ILIKE ? ORDER BY id");
    // Escape ILIKE's own wildcard metacharacters (and the escape character
    // itself) in the user's raw search text — otherwise a query containing
    // "%" or "_" would match far more (or, for a literal underscore/percent
    // the user is actually searching for, differently) than the substring
    // they typed.
    QString escaped = searchQuery;
    escaped.replace("\\", "\\\\").replace("%", "\\%").replace("_", "\\_");
    QString pattern = "%" + escaped + "%";
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    query.addBindValue(pattern);
    
    if (!query.exec()) {
        return donors;
    }
    
    while (query.next()) {
        Donor donor;
        donor.setId(query.value(0).toInt());
        donor.setName(query.value(1).toString().toStdString());
        donor.setEmail(query.value(2).toString().toStdString());
        donor.setPhone(query.value(3).toString().toStdString());
        donor.setAddress(query.value(4).toString().toStdString());
        donor.setNotes(query.value(5).toString().toStdString());
        donor.setRegistrationDate(query.value(6).toString().toStdString());
        donors.push_back(donor);
    }
    
    return donors;
}


std::vector<Donation> Repository::getAllDonations() {
    std::vector<Donation> donations;
    QSqlQuery query(db_.getConnection());
    
    if (!query.exec("SELECT id, donor_id, project_id, amount, donation_date, payment_method, notes FROM donations ORDER BY donation_date DESC")) {
        return donations;
    }
    
    while (query.next()) {
        Donation donation;
        donation.setId(query.value(0).toInt());
        donation.setDonorId(query.value(1).toInt());
        donation.setProjectId(query.value(2).toInt());
        donation.setAmount(query.value(3).toDouble());
        donation.setDate(query.value(4).toString().toStdString());
        donation.setPaymentMethod(query.value(5).toString().toStdString());
        donation.setNotes(query.value(6).toString().toStdString());
        donations.push_back(donation);
    }
    
    return donations;
}

std::optional<Donation> Repository::getDonationById(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("SELECT id, donor_id, project_id, amount, donation_date, payment_method, notes FROM donations WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }
    
    Donation donation;
    donation.setId(query.value(0).toInt());
    donation.setDonorId(query.value(1).toInt());
    donation.setProjectId(query.value(2).toInt());
    donation.setAmount(query.value(3).toDouble());
    donation.setDate(query.value(4).toString().toStdString());
    donation.setPaymentMethod(query.value(5).toString().toStdString());
    donation.setNotes(query.value(6).toString().toStdString());
    return donation;
}

bool Repository::addDonation(Donation& donation) {
    QSqlQuery query(db_.getConnection());
    query.prepare("INSERT INTO donations (donor_id, project_id, amount, donation_date, payment_method, notes) "
                  "VALUES (?, ?, ?, ?, ?, ?) RETURNING id");
    query.addBindValue(donation.getDonorId());
    if (donation.getProjectId() > 0) {
        query.addBindValue(donation.getProjectId());
    } else {
        query.addBindValue(QVariant(QMetaType(QMetaType::Int)));
    }
    query.addBindValue(donation.getAmount());
    query.addBindValue(QString::fromStdString(donation.getDate()));
    query.addBindValue(QString::fromStdString(donation.getPaymentMethod()));
    query.addBindValue(QString::fromStdString(donation.getNotes()));
    if (query.exec() && query.next()) {
        donation.setId(query.value(0).toInt());
        return true;
    }
    setLastError(query);
    return false;
}

bool Repository::updateDonation(const Donation& donation) {
    QSqlQuery query(db_.getConnection());
    query.prepare("UPDATE donations SET donor_id = ?, project_id = ?, amount = ?, donation_date = ?, payment_method = ?, notes = ? WHERE id = ?");
    query.addBindValue(donation.getDonorId());
    if (donation.getProjectId() > 0) {
        query.addBindValue(donation.getProjectId());
    } else {
        query.addBindValue(QVariant(QMetaType(QMetaType::Int)));
    }
    query.addBindValue(donation.getAmount());
    query.addBindValue(QString::fromStdString(donation.getDate()));
    query.addBindValue(QString::fromStdString(donation.getPaymentMethod()));
    query.addBindValue(QString::fromStdString(donation.getNotes()));
    query.addBindValue(donation.getId());

    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}

bool Repository::deleteDonation(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("DELETE FROM donations WHERE id = ?");
    query.addBindValue(id);
    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}

std::vector<Donation> Repository::getDonationsByDonor(int donorId) {
    std::vector<Donation> donations;
    QSqlQuery query(db_.getConnection());
    
    query.prepare("SELECT id, donor_id, project_id, amount, donation_date, payment_method, notes FROM donations WHERE donor_id = ? ORDER BY donation_date DESC");
    query.addBindValue(donorId);
    
    if (!query.exec()) {
        return donations;
    }
    
    while (query.next()) {
        Donation donation;
        donation.setId(query.value(0).toInt());
        donation.setDonorId(query.value(1).toInt());
        donation.setProjectId(query.value(2).toInt());
        donation.setAmount(query.value(3).toDouble());
        donation.setDate(query.value(4).toString().toStdString());
        donation.setPaymentMethod(query.value(5).toString().toStdString());
        donation.setNotes(query.value(6).toString().toStdString());
        donations.push_back(donation);
    }
    
    return donations;
}

std::vector<Donation> Repository::getDonationsByProject(int projectId) {
    std::vector<Donation> donations;
    QSqlQuery query(db_.getConnection());
    
    query.prepare("SELECT id, donor_id, project_id, amount, donation_date, payment_method, notes FROM donations WHERE project_id = ? ORDER BY donation_date DESC");
    query.addBindValue(projectId);
    
    if (!query.exec()) {
        return donations;
    }
    
    while (query.next()) {
        Donation donation;
        donation.setId(query.value(0).toInt());
        donation.setDonorId(query.value(1).toInt());
        donation.setProjectId(query.value(2).toInt());
        donation.setAmount(query.value(3).toDouble());
        donation.setDate(query.value(4).toString().toStdString());
        donation.setPaymentMethod(query.value(5).toString().toStdString());
        donation.setNotes(query.value(6).toString().toStdString());
        donations.push_back(donation);
    }
    
    return donations;
}


std::vector<Project> Repository::getAllProjects() {
    std::vector<Project> projects;
    QSqlQuery query(db_.getConnection());
    
    if (!query.exec("SELECT id, name, description, goal_amount, current_amount, start_date, end_date, status FROM projects ORDER BY id")) {
        return projects;
    }
    
    while (query.next()) {
        Project project;
        project.setId(query.value(0).toInt());
        project.setName(query.value(1).toString().toStdString());
        project.setDescription(query.value(2).toString().toStdString());
        project.setGoalAmount(query.value(3).toDouble());
        project.setCurrentAmount(query.value(4).toDouble());
        project.setStartDate(query.value(5).toString().toStdString());
        project.setEndDate(query.value(6).toString().toStdString());
        project.setStatus(query.value(7).toString().toStdString());
        projects.push_back(project);
    }
    
    return projects;
}

std::optional<Project> Repository::getProjectById(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("SELECT id, name, description, goal_amount, current_amount, start_date, end_date, status FROM projects WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }
    
    Project project;
    project.setId(query.value(0).toInt());
    project.setName(query.value(1).toString().toStdString());
    project.setDescription(query.value(2).toString().toStdString());
    project.setGoalAmount(query.value(3).toDouble());
    project.setCurrentAmount(query.value(4).toDouble());
    project.setStartDate(query.value(5).toString().toStdString());
    project.setEndDate(query.value(6).toString().toStdString());
    project.setStatus(query.value(7).toString().toStdString());
    
    return project;
}

bool Repository::addProject(Project& project) {
    QSqlQuery query(db_.getConnection());
    query.prepare("INSERT INTO projects (name, description, goal_amount, start_date, end_date, status) "
                  "VALUES (?, ?, ?, ?, ?, ?) RETURNING id");
    query.addBindValue(QString::fromStdString(project.getName()));
    query.addBindValue(QString::fromStdString(project.getDescription()));
    query.addBindValue(project.getGoalAmount());
    query.addBindValue(QString::fromStdString(project.getStartDate()));
    query.addBindValue(QString::fromStdString(project.getEndDate()));
    query.addBindValue(QString::fromStdString(project.getStatus()));
    
    if (query.exec() && query.next()) {
        project.setId(query.value(0).toInt());
        return true;
    }
    setLastError(query);
    return false;
}

bool Repository::updateProject(const Project& project) {
    QSqlQuery query(db_.getConnection());
    query.prepare("UPDATE projects SET name = ?, description = ?, goal_amount = ?, start_date = ?, end_date = ?, status = ? WHERE id = ?");
    query.addBindValue(QString::fromStdString(project.getName()));
    query.addBindValue(QString::fromStdString(project.getDescription()));
    query.addBindValue(project.getGoalAmount());
    query.addBindValue(QString::fromStdString(project.getStartDate()));
    query.addBindValue(QString::fromStdString(project.getEndDate()));
    query.addBindValue(QString::fromStdString(project.getStatus()));
    query.addBindValue(project.getId());

    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}

bool Repository::deleteProject(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("DELETE FROM projects WHERE id = ?");
    query.addBindValue(id);
    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}

std::vector<Project> Repository::getActiveProjects() {
    std::vector<Project> projects;
    QSqlQuery query(db_.getConnection());
    
    if (!query.exec("SELECT id, name, description, goal_amount, current_amount, start_date, end_date, status FROM projects WHERE status = 'active' ORDER BY id")) {
        return projects;
    }
    
    while (query.next()) {
        Project project;
        project.setId(query.value(0).toInt());
        project.setName(query.value(1).toString().toStdString());
        project.setDescription(query.value(2).toString().toStdString());
        project.setGoalAmount(query.value(3).toDouble());
        project.setCurrentAmount(query.value(4).toDouble());
        project.setStartDate(query.value(5).toString().toStdString());
        project.setEndDate(query.value(6).toString().toStdString());
        project.setStatus(query.value(7).toString().toStdString());
        projects.push_back(project);
    }
    
    return projects;
}


std::vector<Beneficiary> Repository::getAllBeneficiaries() {
    std::vector<Beneficiary> beneficiaries;
    QSqlQuery query(db_.getConnection());
    
    if (!query.exec("SELECT id, name, contact, project_id, description, assistance_type FROM beneficiaries ORDER BY id")) {
        return beneficiaries;
    }
    
    while (query.next()) {
        Beneficiary beneficiary;
        beneficiary.setId(query.value(0).toInt());
        beneficiary.setName(query.value(1).toString().toStdString());
        beneficiary.setContact(query.value(2).toString().toStdString());
        beneficiary.setProjectId(query.value(3).toInt());
        beneficiary.setDescription(query.value(4).toString().toStdString());
        beneficiary.setAssistanceType(query.value(5).toString().toStdString());
        beneficiaries.push_back(beneficiary);
    }
    
    return beneficiaries;
}

std::optional<Beneficiary> Repository::getBeneficiaryById(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("SELECT id, name, contact, project_id, description, assistance_type FROM beneficiaries WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec() || !query.next()) {
        return std::nullopt;
    }
    
    Beneficiary beneficiary;
    beneficiary.setId(query.value(0).toInt());
    beneficiary.setName(query.value(1).toString().toStdString());
    beneficiary.setContact(query.value(2).toString().toStdString());
    beneficiary.setProjectId(query.value(3).toInt());
    beneficiary.setDescription(query.value(4).toString().toStdString());
    beneficiary.setAssistanceType(query.value(5).toString().toStdString());
    
    return beneficiary;
}

bool Repository::addBeneficiary(Beneficiary& beneficiary) {
    QSqlQuery query(db_.getConnection());
    query.prepare("INSERT INTO beneficiaries (name, contact, project_id, description, assistance_type) "
                  "VALUES (?, ?, ?, ?, ?) RETURNING id");
    query.addBindValue(QString::fromStdString(beneficiary.getName()));
    query.addBindValue(QString::fromStdString(beneficiary.getContact()));
    if (beneficiary.getProjectId() > 0) {
        query.addBindValue(beneficiary.getProjectId());
    } else {
        query.addBindValue(QVariant(QMetaType(QMetaType::Int)));
    }
    query.addBindValue(QString::fromStdString(beneficiary.getDescription()));
    query.addBindValue(QString::fromStdString(beneficiary.getAssistanceType()));

    if (query.exec() && query.next()) {
        beneficiary.setId(query.value(0).toInt());
        return true;
    }
    setLastError(query);
    return false;
}

bool Repository::updateBeneficiary(const Beneficiary& beneficiary) {
    QSqlQuery query(db_.getConnection());
    query.prepare("UPDATE beneficiaries SET name = ?, contact = ?, project_id = ?, description = ?, assistance_type = ? WHERE id = ?");
    query.addBindValue(QString::fromStdString(beneficiary.getName()));
    query.addBindValue(QString::fromStdString(beneficiary.getContact()));
    if (beneficiary.getProjectId() > 0) {
        query.addBindValue(beneficiary.getProjectId());
    } else {
        query.addBindValue(QVariant(QMetaType(QMetaType::Int)));
    }
    query.addBindValue(QString::fromStdString(beneficiary.getDescription()));
    query.addBindValue(QString::fromStdString(beneficiary.getAssistanceType()));
    query.addBindValue(beneficiary.getId());

    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}

bool Repository::deleteBeneficiary(int id) {
    QSqlQuery query(db_.getConnection());
    query.prepare("DELETE FROM beneficiaries WHERE id = ?");
    query.addBindValue(id);
    bool ok = query.exec();
    if (!ok) setLastError(query);
    return ok;
}


double Repository::getTotalDonations() {
    QSqlQuery query(db_.getConnection());
    
    if (!query.exec("SELECT COALESCE(SUM(amount), 0) FROM donations")) {
        return 0.0;
    }
    
    if (query.next()) {
        return query.value(0).toDouble();
    }
    
    return 0.0;
}

double Repository::getTotalDonationsByDonor(int donorId) {
    QSqlQuery query(db_.getConnection());
    query.prepare("SELECT COALESCE(SUM(amount), 0) FROM donations WHERE donor_id = ?");
    query.addBindValue(donorId);
    
    if (!query.exec() || !query.next()) {
        return 0.0;
    }
    
    return query.value(0).toDouble();
}

int Repository::getDonationCountByDonor(int donorId) {
    QSqlQuery query(db_.getConnection());
    query.prepare("SELECT COUNT(*) FROM donations WHERE donor_id = ?");
    query.addBindValue(donorId);

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

std::vector<DonorStat> Repository::getDonorStatistics() {
    std::vector<DonorStat> stats;
    QSqlQuery query(db_.getConnection());

    if (!query.exec("SELECT id, name, total_donations, total_amount, last_donation_date FROM donor_statistics")) {
        setLastError(query);
        return stats;
    }

    while (query.next()) {
        DonorStat stat;
        stat.donorId = query.value(0).toInt();
        stat.donorName = query.value(1).toString();
        stat.totalDonations = query.value(2).toInt();
        stat.totalAmount = query.value(3).toDouble();
        stat.lastDonationDate = query.value(4).toString();
        stats.push_back(stat);
    }

    return stats;
}

std::vector<DonationView> Repository::getAllDonationsWithNames() {
    std::vector<DonationView> views;
    QSqlQuery query(db_.getConnection());

    if (!query.exec("SELECT id, donor_name, project_name, amount, donation_date, payment_method, notes "
                     "FROM donation_summary")) {
        setLastError(query);
        return views;
    }

    while (query.next()) {
        DonationView view;
        view.id = query.value(0).toInt();
        view.donorName = query.value(1).toString();
        view.projectName = query.value(2).isNull() ? QString("Без проекту") : query.value(2).toString();
        view.amount = query.value(3).toDouble();
        view.date = query.value(4).toString();
        view.paymentMethod = query.value(5).toString();
        view.notes = query.value(6).toString();
        views.push_back(view);
    }

    return views;
}
