#include "Lang.h"
#include <QSettings>
#include <QHash>
#include <QList>
#include <QPair>
#include <algorithm>

namespace {

bool g_loaded = false;
L::Code g_code = L::Uk;

void ensureLoaded() {
    if (g_loaded) return;
    QSettings settings("CharityFund", "CharityFund");
    g_code = settings.value("ui/language", "uk").toString() == "en" ? L::En : L::Uk;
    g_loaded = true;
}

const QList<QPair<QString, QString>>& dictionary() {
    static const QList<QPair<QString, QString>> dict = {
        // ---- Window / menus / toolbar ----
        {"Система обліку благодійного фонду", "Charity Fund Accounting System"},
        {"&Файл", "&File"},
        {"&Довідка", "&Help"},
        {"&Вигляд", "&View"},
        {"Оновити", "Refresh"},
        {"Вихід", "Exit"},
        {"Перевірити БД", "Check Database"},
        {"Про програму", "About"},
        {"Основна панель", "Main Toolbar"},
        {"Світла тема", "Light Theme"},
        {"Темна тема", "Dark Theme"},

        // ---- Status bar / DB dialogs ----
        {"Статус БД", "Database Status"},
        {"Помилка БД", "Database Error"},
        {"Підключено до БД", "Connected to DB"},
        {"Загальна сума пожертв", "Total donations"},
        {"Немає підключення до БД", "No database connection"},
        {"Підключення до бази даних активне", "Database connection is active"},
        {"Немає підключення до бази даних", "No database connection"},
        {"БД:", "Database:"},
        {"Хост:", "Host:"},
        {"Перевірте:", "Please check:"},
        {"PostgreSQL запущений", "PostgreSQL is running"},
        {"База даних 'charity_fund' існує", "The 'charity_fund' database exists"},
        {"Користувач 'charity_user' має права доступу", "The 'charity_user' role has access rights"},
        {"Помилка: ", "Error: "},
        {"Помилка підключення", "Connection Error"},
        {"Не вдалося підключитися до бази даних!", "Failed to connect to the database!"},
        {"1. PostgreSQL запущений", "1. PostgreSQL is running"},
        {"2. База даних 'charity_fund' створена", "2. The 'charity_fund' database is created"},
        {"3. Користувач 'charity_user' має права", "3. The 'charity_user' role has access rights"},

        // ---- About dialog ----
        {"Версія:", "Version:"},
        {"Курсова робота", "Coursework project"},
        {"з основ програмування", "for the Fundamentals of Programming course"},
        {"Функціонал:", "Features:"},
        {"Облік донорів (благодійників)", "Donor (benefactor) tracking"},
        {"Реєстрація та облік пожертв", "Donation registration and tracking"},
        {"Управління благодійними проектами", "Charity project management"},
        {"Облік бенефіціарів", "Beneficiary tracking"},
        {"Генерація звітів", "Report generation"},
        {"Технології:", "Technologies:"},

        // ---- Tabs ----
        {"Донори", "Donors"},
        {"Пожертви", "Donations"},
        {"Проекти", "Projects"},
        {"Бенефіціари", "Beneficiaries"},
        {"Звіти", "Reports"},

        // ---- Donors tab ----
        {"Донори (благодійники)", "Donors (benefactors)"},
        {"Приховати форму", "Hide form"},
        {"Показати форму", "Show form"},
        {"Пошук:", "Search:"},
        {"Введіть ім'я, email або телефон...", "Enter name, email or phone..."},
        {"Пошук за донором, проектом чи приміткою...", "Search by donor, project or note..."},
        {"Пошук за назвою, описом чи статусом...", "Search by name, description or status..."},
        {"Пошук за ім'ям, контактом чи типом допомоги...", "Search by name, contact or assistance type..."},
        {"Ім'я", "Name"},
        {"Телефон", "Phone"},
        {"Адреса", "Address"},
        {"Ім'я:", "Name:"},
        {"Email:", "Email:"},
        {"Додати/Редагувати донора", "Add/Edit Donor"},
        {"Обов'язкове поле", "Required field"},
        {"м. Київ, вул...", "City, street..."},
        {"Додаткова інформація", "Additional information"},
        {"Додати", "Add"},
        {"Редагувати", "Edit"},
        {"Видалити", "Delete"},
        {"Очистити", "Clear"},
        {"Помилка", "Error"},
        {"Будь ласка, введіть ім'я донора!", "Please enter the donor's name!"},
        {"Некоректні дані! Перевірте email.", "Invalid data! Check the email."},
        {"Успіх", "Success"},
        {"Донора додано успішно!", "Donor added successfully!"},
        {"Не вдалося додати донора!", "Failed to add donor!"},
        {"Спочатку виберіть донора з таблиці!", "Please select a donor from the table first!"},
        {"Ім'я не може бути порожнім!", "Name cannot be empty!"},
        {"Дані донора оновлено!", "Donor data updated!"},
        {"Не вдалося оновити донора!", "Failed to update donor!"},
        {"Підтвердження", "Confirmation"},
        {"Ви впевнені, що хочете видалити цього донора?\nВсі пов'язані пожертви також будуть видалені!",
         "Are you sure you want to delete this donor?\nAll related donations will also be deleted!"},
        {"Донора видалено!", "Donor deleted!"},
        {"Не вдалося видалити донора!", "Failed to delete donor!"},

        // ---- Donations tab ----
        {"Облік пожертв", "Donation Tracking"},
        {"Донор", "Donor"},
        {"Проект", "Project"},
        {"Сума (грн)", "Amount (UAH)"},
        {"Дата", "Date"},
        {"Спосіб оплати", "Payment Method"},
        {"Примітки", "Notes"},
        {"Зареєструвати пожертву", "Register Donation"},
        {"Донор:", "Donor:"},
        {"Без проекту", "No project"},
        {"Проект:", "Project:"},
        {"Сума (грн):", "Amount (UAH):"},
        {"Дата:", "Date:"},
        {"Спосіб оплати:", "Payment method:"},
        {"Примітки:", "Notes:"},
        {"Додати пожертву", "Add Donation"},
        {"Спочатку додайте хоча б одного донора!", "Please add at least one donor first!"},
        {"Введіть коректну суму (більше 0)!", "Enter a valid amount (greater than 0)!"},
        {"Некоректні дані пожертви!", "Invalid donation data!"},
        {"Пожертву зареєстровано!", "Donation registered!"},
        {"Не вдалося додати пожертву!", "Failed to add donation!"},
        {"Виберіть пожертву для видалення!", "Select a donation to delete!"},
        {"Видалити цю пожертву?", "Delete this donation?"},
        {"Пожертву видалено!", "Donation deleted!"},
        {"Не вдалося видалити пожертву!", "Failed to delete donation!"},

        // ---- Projects tab ----
        {"Благодійні проекти", "Charity Projects"},
        {"Назва", "Name"},
        {"Ціль (грн)", "Goal (UAH)"},
        {"Зібрано (грн)", "Raised (UAH)"},
        {"Прогрес", "Progress"},
        {"Початок", "Start"},
        {"Статус", "Status"},
        {"Опис", "Description"},
        {"Активний", "Active"},
        {"active", "Active"},
        {"Створити новий проект", "Create New Project"},
        {"Назва:", "Name:"},
        {"Назва проекту", "Project name"},
        {"Опис:", "Description:"},
        {"Опис проекту", "Project description"},
        {"Ціль (грн):", "Goal (UAH):"},
        {"Дата початку:", "Start date:"},
        {"Дата завершення:", "End date:"},
        {"Створити проект", "Create Project"},
        {"Введіть назву проекту!", "Enter the project name!"},
        {"Введіть коректну цільову суму!", "Enter a valid goal amount!"},
        {"Некоректні дані проекту! Перевірте назву та дати (дата завершення не може бути раніше дати початку).",
         "Invalid project data! Check the name and dates (end date cannot be earlier than start date)."},
        {"Проект створено!", "Project created!"},
        {"Не вдалося створити проект!", "Failed to create project!"},
        {"Виберіть проект для видалення!", "Select a project to delete!"},
        {"Видалити цей проект?\nПожертви та бенефіціари будуть збережені, але прив'язка до проекту буде видалена.",
         "Delete this project?\nDonations and beneficiaries will be kept, but their link to the project will be removed."},
        {"Проект видалено!", "Project deleted!"},
        {"Не вдалося видалити проект!", "Failed to delete project!"},
        {"Деталі проекту", "Project Details"},
        {"Деталі", "Details"},
        {"Ціль:", "Goal:"},
        {"Зібрано:", "Raised:"},
        {"Прогрес:", "Progress:"},
        {"Період:", "Period:"},
        {"Статус:", "Status:"},

        // ---- Beneficiaries tab ----
        {"Бенефіціари (отримувачі допомоги)", "Beneficiaries (aid recipients)"},
        {"Бенефіціари", "Beneficiaries"},
        {"Контакт", "Contact"},
        {"Тип допомоги", "Assistance Type"},
        {"Додати/Редагувати бенефіціара", "Add/Edit Beneficiary"},
        {"Телефон або email", "Phone or email"},
        {"Контакт:", "Contact:"},
        {"Тип допомоги:", "Assistance type:"},
        {"Напр. Фінансова підтримка", "e.g. Financial support"},
        {"Будь ласка, введіть ім'я бенефіціара!", "Please enter the beneficiary's name!"},
        {"Некоректні дані бенефіціара!", "Invalid beneficiary data!"},
        {"Бенефіціара додано успішно!", "Beneficiary added successfully!"},
        {"Не вдалося додати бенефіціара!", "Failed to add beneficiary!"},
        {"Спочатку виберіть бенефіціара з таблиці!", "Please select a beneficiary from the table first!"},
        {"Дані бенефіціара оновлено!", "Beneficiary data updated!"},
        {"Не вдалося оновити бенефіціара!", "Failed to update beneficiary!"},
        {"Ви впевнені, що хочете видалити цього бенефіціара?", "Are you sure you want to delete this beneficiary?"},
        {"Бенефіціара видалено!", "Beneficiary deleted!"},
        {"Не вдалося видалити бенефіціара!", "Failed to delete beneficiary!"},

        // ---- Reports tab ----
        {"Звіти та статистика", "Reports & Statistics"},
        {"Тип звіту:", "Report type:"},
        {"Загальна статистика", "General Statistics"},
        {"Статистика по донорам", "Donor Statistics"},
        {"Статистика по проектам", "Project Statistics"},
        {"Згенерувати звіт", "Generate Report"},
        {"Експорт в файл", "Export to File"},
        {"Ціль", "Goal"},
        {"Зібрано", "Raised"},
        {"Зібрано по проектах", "Raised by Project"},
        {"Інші", "Others"},
        {"Розподіл пожертв по донорах", "Donation Distribution by Donor"},
        {"Невідомий тип звіту", "Unknown report type"},
        {"Загальний фінансовий звіт", "General Financial Report"},
        {"Згенеровано:", "Generated:"},
        {"Фінанси", "Finances"},
        {"Загальна сума пожертв:", "Total donations:"},
        {"грн", "UAH"},
        {"Кількість пожертв:", "Number of donations:"},
        {"Середня пожертва:", "Average donation:"},
        {"Всього донорів:", "Total donors:"},
        {"Всього проектів:", "Total projects:"},
        {"Активних проектів:", "Active projects:"},
        {"Загальна ціль:", "Total goal:"},
        {"Зібрано для проектів:", "Raised for projects:"},
        {"Всього пожертв", "Total donations"},
        {"Загальна сума (грн)", "Total Amount (UAH)"},
        {"Остання пожертва", "Last Donation"},
        {"Зберегти звіт", "Save Report"},
        {"PDF файли (*.pdf);;HTML файли (*.html);;Текстові файли (*.txt)",
         "PDF Files (*.pdf);;HTML Files (*.html);;Text Files (*.txt)"},
        {"Звіт - Charity Fund", "Report - Charity Fund"},
        {"Звіт збережено:", "Report saved:"},
        {"Не вдалося зберегти файл!", "Failed to save the file!"},

        // ---- Sidebar / header shell ----
        {"Огляд", "Overview"},
        {"ОБЛІК", "RECORDS"},
        {"АНАЛІТИКА", "ANALYTICS"},
        {"Назва фонду", "Fund name"},
        {"Акцент", "Accent"},
        {"Фонд «Милосердя»", "Mercy Foundation"},
        {"Звітний рік 2026", "Reporting year 2026"},
        {"АД", "AD"},
        {"Адміністратор", "Administrator"},
        {"Керівник фонду", "Fund Manager"},

        // ---- Dashboard (Огляд) ----
        {"Огляд фонду", "Fund overview"},
        {"Ключові показники за поточний звітний період", "Key figures for the current reporting period"},
        {"Зібрано всього", "Total raised"},
        {"Виконання проектів", "Project delivery"},
        {"Останні пожертви", "Recent donations"},
        {"активні", "active"},
        {"завершено", "completed"},
        {"середня", "average"},
        {"Немає даних", "No data"},
        // Exact dashboard KPI captions — registered explicitly so the exact-match
        // lookup in t() wins before the substring-based html() fallback, which
        // would otherwise mangle these (e.g. "Донорів" contains "Донор" as a
        // substring and gets a partial Cyrillic/Latin mix like "DonorІВ").
        {"Пожертв", "Donations"},
        {"Донорів", "Donors"},

        // ---- Payment method / project status display labels ----
        {"Готівка", "Cash"},
        {"Переказ", "Transfer"},
        {"Картка", "Card"},
        {"Онлайн", "Online"},
        {"Завершено", "Completed"},
    };
    return dict;
}

const QHash<QString, QString>& exactIndex() {
    static const QHash<QString, QString> index = [] {
        QHash<QString, QString> h;
        for (const auto& pair : dictionary()) h.insert(pair.first, pair.second);
        return h;
    }();
    return index;
}

const QList<QPair<QString, QString>>& byLengthDesc() {
    static const QList<QPair<QString, QString>> sorted = [] {
        QList<QPair<QString, QString>> list = dictionary();
        std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
            return a.first.length() > b.first.length();
        });
        return list;
    }();
    return sorted;
}

} // namespace

namespace L {

Code code() {
    ensureLoaded();
    return g_code;
}

bool isUk() {
    return code() == Uk;
}

void setCode(Code c) {
    ensureLoaded();
    g_code = c;
    QSettings settings("CharityFund", "CharityFund");
    settings.setValue("ui/language", c == En ? "en" : "uk");
}

void toggle() {
    setCode(isUk() ? En : Uk);
}

QString html(const QString& uk) {
    if (isUk()) return uk;

    QString result = uk;
    for (const auto& pair : byLengthDesc()) {
        if (pair.first.length() <= 1) continue; // skip single-glyph keys — see Lang.h
        if (result.contains(pair.first)) {
            result.replace(pair.first, pair.second);
        }
    }
    return result;
}

QString t(const QString& uk) {
    if (isUk()) return uk;

    const auto& index = exactIndex();
    auto it = index.find(uk);
    if (it != index.end()) return it.value();

    QString trimmed = uk.trimmed();
    if (trimmed != uk) {
        auto trimmedIt = index.find(trimmed);
        if (trimmedIt != index.end()) {
            QString leading = uk.left(uk.indexOf(trimmed));
            QString trailing = uk.mid(uk.indexOf(trimmed) + trimmed.length());
            return leading + trimmedIt.value() + trailing;
        }
    }

    return html(uk);
}

QString t(const char* uk) {
    return t(QString::fromUtf8(uk));
}

QString paymentMethodLabel(const QString& code) {
    static const QHash<QString, QString> kMap = {
        {"cash", QString::fromUtf8("Готівка")},
        {"bank_transfer", QString::fromUtf8("Переказ")},
        {"card", QString::fromUtf8("Картка")},
        {"online", QString::fromUtf8("Онлайн")},
    };
    return t(kMap.value(code, code));
}

QString projectStatusLabel(const QString& code) {
    static const QHash<QString, QString> kMap = {
        {"active", QString::fromUtf8("Активний")},
        {"completed", QString::fromUtf8("Завершено")},
    };
    return t(kMap.value(code, code));
}

} // namespace L
