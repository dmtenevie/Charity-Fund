#include "ReportsWidget.h"
#include "../Theme.h"
#include "../Lang.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QBarSet>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QAbstractAxis>
#include <QPieSeries>
#include <QPieSlice>
#include <QPainter>
#include <QLocale>
#include <QPrinter>

namespace {
QString money(double v) {
    QLocale ua(QLocale::Ukrainian, QLocale::Ukraine);
    return ua.toString(v, 'f', 2);
}

// Colors for the printable report documents. Intentionally NOT theme-aware
// (unlike the rest of the app): a report is a document meant to be read on
// screen, exported to PDF/HTML, and printed on paper, and paper/PDF pages
// are always white. If this followed the dark theme, report text would be
// near-white (readable only against the app's own dark background) and turn
// invisible the moment it's exported or printed onto an actual white page.
// So reports always render black-on-white, regardless of the app's theme.
struct ReportPalette {
    QString heading, muted, accent, green, yellow, lowProgress, headerBg, rowA, rowB, cardBg, border;

    static ReportPalette current() {
        ReportPalette p;
        p.heading     = "#191817";
        p.muted       = "#6a655e";
        p.accent      = "#5c8a5c";
        p.green       = "#3f8f6f";
        p.yellow      = "#b0812a";
        // Distinct from p.accent/p.green (both green) — used for the
        // "needs funding" progress state so it doesn't visually collide
        // with the "fully funded" state below.
        p.lowProgress = "#4a7ba3";
        p.headerBg    = "#f2f0ec";
        p.rowA        = "#faf8f5";
        p.rowB        = "#ffffff";
        p.cardBg      = "#f7f5f1";
        p.border      = "#e4e0d9";
        return p;
    }
};

// Letterhead every report opens with: fund identity on the left, report
// title + generation timestamp on the right — the header block that real
// accounting/CRM reports (invoices, statements, donor reports) always lead
// with, instead of a bare heading.
// Single-column (fund identity, then title/date below a rule) rather than a
// two-column letterhead — the report panel can be quite narrow (it shares a
// splitter with the charts, and the user can drag it narrower still), and a
// side-by-side layout squeezed each side into a vertical stack of single
// characters at low widths. Stacked blocks stay readable at any width.
QString reportLetterhead(const QString& title, const ReportPalette& p) {
    return QString(
        "<div><span style='font-size: 15pt; font-weight: 700; color: %2;'>%4</span><br>"
        "<span style='font-size: 8.5pt; color: %3;'>%5</span></div>"
        "<div style='border-top: 2px solid %1; margin-top: 8px; padding-top: 8px;'>"
        "<span style='font-size: 12pt; font-weight: 600; color: %2;'>%6</span><br>"
        "<span style='font-size: 8.5pt; color: %3;'>%7 %8</span></div>"
    ).arg(p.accent, p.heading, p.muted, L::t("Фонд «Милосердя»"))
     .arg(L::t("Система обліку благодійного фонду"), title, L::t("Згенеровано:"),
          QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm"));
}

// Closing strip every report ends with — same branding as the letterhead,
// so a printed/exported page is identifiable even without its first page.
QString reportFooter(const ReportPalette& p) {
    return QString(
        "<table width='100%' cellpadding='0' cellspacing='0' style='margin-top: 18px;'><tr>"
        "<td style='border-top: 1px solid %1; padding-top: 8px;'>"
        "<span style='font-size: 8pt; color: %2;'>%3 · %4</span>"
        "</td></tr></table>"
    ).arg(p.border, p.muted, L::t("Фонд «Милосердя»"), L::t("Система обліку благодійного фонду"));
}

// One KPI tile (big number + caption, colored top edge). Four of these form
// the at-a-glance summary every professional report opens its body with,
// laid out as a 2x2 grid rather than 4-across — at 50% width each they stay
// readable even when the report panel (which shares a splitter with the
// charts, and can be dragged narrower by the user) is fairly narrow.
QString kpiCard(const QString& value, const QString& caption, const QString& barColor, const ReportPalette& p) {
    return QString(
        "<td width='50%' style='padding: 3px;'>"
        "<table width='100%' cellpadding='0' cellspacing='0' style='background-color:%1; border-top: 3px solid %2;'>"
        "<tr><td style='padding: 10px 12px;'>"
        "<div style='font-size: 8pt; color:%3;'>%4</div>"
        "<div style='font-size: 14pt; font-weight: 700; color:%5; padding-top: 3px;'>%6</div>"
        "</td></tr></table></td>"
    ).arg(p.cardBg, barColor, p.muted, caption, p.heading, value);
}

// Themed open tag for a bordered data table — replaces the old
// <table border='1'> (a hardcoded black bevel that clashed with the dark
// theme) with a border color drawn from the current palette.
QString tableOpen(const ReportPalette& p, const QString& extraStyle = QString()) {
    return QString("<table width='100%' cellpadding='8' cellspacing='0' "
                    "style='border-collapse: collapse; border: 1px solid %1;%2'>")
        .arg(p.border, extraStyle);
}
} // namespace

ReportsWidget::ReportsWidget(Repository* repo, QWidget* parent)
    : QWidget(parent), repository_(repo) {
    setupUI();
}

void ReportsWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QLabel* titleLabel = new QLabel(L::t("Звіти та статистика"));
    QFont font = titleLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    titleLabel->setFont(font);
    mainLayout->addWidget(titleLabel);
    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->addWidget(new QLabel(L::t("Тип звіту:")));
    reportTypeCombo_ = new QComboBox();
    reportTypeCombo_->addItem(L::t("Загальна статистика"), 0);
    reportTypeCombo_->addItem(L::t("Статистика по донорам"), 1);
    reportTypeCombo_->addItem(L::t("Статистика по проектам"), 2);
    controlLayout->addWidget(reportTypeCombo_);
    QPushButton* generateBtn = new QPushButton(L::t("Згенерувати звіт"));
    generateBtn->setObjectName("primaryButton");
    connect(generateBtn, &QPushButton::clicked, this, &ReportsWidget::generateReport);
    controlLayout->addWidget(generateBtn);
    QPushButton* exportBtn = new QPushButton(L::t("Експорт в файл"));
    connect(exportBtn, &QPushButton::clicked, this, &ReportsWidget::exportReport);
    controlLayout->addWidget(exportBtn);
    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);

    QSplitter* splitter = new QSplitter(Qt::Horizontal);

    QWidget* chartsPanel = new QWidget();
    QVBoxLayout* chartsLayout = new QVBoxLayout(chartsPanel);
    chartsLayout->setContentsMargins(0, 0, 0, 0);

    projectsChartView_ = new QChartView();
    projectsChartView_->setRenderHint(QPainter::Antialiasing);
    projectsChartView_->setMinimumHeight(220);
    chartsLayout->addWidget(projectsChartView_);

    donorsChartView_ = new QChartView();
    donorsChartView_->setRenderHint(QPainter::Antialiasing);
    donorsChartView_->setMinimumHeight(220);
    chartsLayout->addWidget(donorsChartView_);

    splitter->addWidget(chartsPanel);

    reportView_ = new QTextEdit();
    reportView_->setObjectName("reportView");
    reportView_->setReadOnly(true);
    // Always black-on-white, independent of the app's dark/light theme (see
    // ReportPalette::current() above) — a widget-local stylesheet overrides
    // the app-level QSS rule for #reportView, so Theme::apply() re-styling
    // the rest of the app on a theme toggle never darkens this widget.
    // `color` must be set explicitly here too: the app-wide dark stylesheet's
    // generic `QTextEdit { color: #eceae7; }` rule (styles.qss) still applies
    // to any property this local sheet doesn't override, so without this the
    // widget's *default* text color stays near-white in dark mode. Report
    // HTML doesn't put an explicit color on every table cell (only the ones
    // that need a specific hue), so those cells inherited that near-white
    // default — nearly invisible against the report's own white background,
    // both on screen and in the exported PDF.
    reportView_->setStyleSheet("QTextEdit#reportView { background-color: #ffffff; color: #191817; border: 1px solid #e4e0d9; border-radius: 12px; }");
    splitter->addWidget(reportView_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    // QChartView's sizeHint tends to dominate an even split on first show,
    // leaving the report document too narrow — pin an explicit starting
    // width for each side instead (still user-resizable afterwards).
    splitter->setSizes({560, 560});

    mainLayout->addWidget(splitter);

    updateCharts();
    generateReport();
}

namespace {
// QChart draws its own background/text via QPainter — it does not follow
// the app's QSS stylesheet, so without this it stays unthemed. Colors come
// from the palette in spec section 1.5.
void styleChartForTheme(QChart* chart) {
    bool dark = Theme::isDark();
    chart->setBackgroundBrush(QBrush(QColor(dark ? "#17191c" : "#ffffff")));
    chart->setBackgroundPen(QPen(QColor(dark ? "#2a2e33" : "#e4e0d9")));
    chart->setTitleBrush(QBrush(QColor(dark ? "#eceae7" : "#191817")));
    if (chart->legend()) {
        chart->legend()->setLabelColor(QColor(dark ? "#a3a29d" : "#6a655e"));
    }
}

void styleAxisForTheme(QAbstractAxis* axis) {
    bool dark = Theme::isDark();
    axis->setLabelsColor(QColor(dark ? "#a3a29d" : "#6a655e"));
    axis->setLinePen(QPen(QColor(dark ? "#2a2e33" : "#e4e0d9")));
    axis->setGridLineColor(QColor(dark ? "#2a2e33" : "#e4e0d9"));
}
}

void ReportsWidget::updateCharts() {
    std::vector<Project> projects = repository_->getAllProjects();

    bool dark = Theme::isDark();
    QString accent   = dark ? "#74a874" : "#5c8a5c";
    QString goalCol  = dark ? "#3a3f45" : "#e0dbd3";
    QString labelCol = dark ? "#a3a29d" : "#6a655e";

    QBarSet* goalSet = new QBarSet(L::t("Ціль"));
    QBarSet* raisedSet = new QBarSet(L::t("Зібрано"));
    goalSet->setColor(QColor(goalCol));
    raisedSet->setColor(QColor(accent));

    QStringList categories;
    for (const auto& project : projects) {
        *goalSet << project.getGoalAmount();
        *raisedSet << project.getCurrentAmount();
        categories << QString::fromStdString(project.getName());
    }

    QBarSeries* barSeries = new QBarSeries();
    barSeries->append(goalSet);
    barSeries->append(raisedSet);

    QChart* projectsChart = new QChart();
    projectsChart->addSeries(barSeries);
    projectsChart->setTitle(L::t("Зібрано по проектах"));
    projectsChart->legend()->setVisible(true);
    projectsChart->legend()->setAlignment(Qt::AlignBottom);
    styleChartForTheme(projectsChart);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    projectsChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    styleAxisForTheme(axisX);

    QValueAxis* axisY = new QValueAxis();
    projectsChart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    styleAxisForTheme(axisY);

    projectsChartView_->setChart(projectsChart);

    std::vector<DonorStat> stats = repository_->getDonorStatistics();
    QPieSeries* pieSeries = new QPieSeries();

    // Sector palette from spec section 1.5 (accent + green + yellow + three fixed
    // hues). Green/yellow are theme-aware; the rest are constant.
    QString green  = dark ? "#5cb894" : "#3f8f6f";
    QString yellow = dark ? "#d9a441" : "#b0812a";
    QColor palette[6] = {
        QColor(accent), QColor(green), QColor(yellow),
        QColor("#8d8880"), QColor("#a98bd0"), QColor("#7fa7d9")
    };

    const int maxSlices = 5;
    double othersTotal = 0.0;
    int colorIndex = 0;
    QColor labelColor(labelCol);
    for (size_t i = 0; i < stats.size(); ++i) {
        if (stats[i].totalAmount <= 0) continue;
        if (static_cast<int>(i) < maxSlices) {
            QPieSlice* slice = pieSeries->append(stats[i].donorName, stats[i].totalAmount);
            slice->setBrush(palette[colorIndex % 6]);
            slice->setLabelColor(labelColor);
            colorIndex++;
        } else {
            othersTotal += stats[i].totalAmount;
        }
    }
    if (othersTotal > 0) {
        QPieSlice* slice = pieSeries->append(L::t("Інші"), othersTotal);
        slice->setBrush(QColor(dark ? "#8d8880" : "#98938b"));
        slice->setLabelColor(labelColor);
    }
    pieSeries->setLabelsVisible(true);

    QChart* donorsChart = new QChart();
    donorsChart->addSeries(pieSeries);
    donorsChart->setTitle(L::t("Розподіл пожертв по донорах"));
    donorsChart->legend()->setVisible(true);
    donorsChart->legend()->setAlignment(Qt::AlignBottom);
    styleChartForTheme(donorsChart);

    donorsChartView_->setChart(donorsChart);
}

void ReportsWidget::generateReport() {
    updateCharts();

    int reportType = reportTypeCombo_->currentData().toInt();
    QString report;

    switch (reportType) {
        case 0:
            report = generateFinancialReport();
            break;
        case 1:
            report = generateDonorStatistics();
            break;
        case 2:
            report = generateProjectStatistics();
            break;
        default:
            report = L::t("Невідомий тип звіту");
    }

    // Translate the fully-composed (Ukrainian) report block to English when
    // needed, longest keys first. The in-app view is the single source for
    // the exported file too.
    // Wrapped in an explicit black-on-white div so every unstyled cell
    // inherits black text no matter what color the QTextEdit itself resolves
    // to (theme QSS, future style changes, etc.) — belt-and-suspenders on
    // top of the widget stylesheet, since this is what actually gets printed
    // to PDF and saved to HTML.
    reportView_->setHtml(QString("<div style='color:#191817; background-color:#ffffff;'>%1</div>")
        .arg(L::html(report)));
}

QString ReportsWidget::generateFinancialReport() {
    double totalDonations = repository_->getTotalDonations();
    std::vector<Donor> donors = repository_->getAllDonors();
    std::vector<Donation> donations = repository_->getAllDonations();
    std::vector<Project> projects = repository_->getAllProjects();

    int activeProjects = 0;
    double totalGoal = 0;
    double totalRaised = 0;

    for (const auto& project : projects) {
        if (project.getStatus() == "active") {
            activeProjects++;
        }
        totalGoal += project.getGoalAmount();
        totalRaised += project.getCurrentAmount();
    }

    double avgDonation = donations.empty() ? 0.0 : totalDonations / donations.size();
    double projectProgress = totalGoal > 0 ? (totalRaised / totalGoal * 100) : 0.0;

    ReportPalette p = ReportPalette::current();

    QString report = reportLetterhead(L::t("Загальний фінансовий звіт"), p);

    // At-a-glance KPI grid — the same four figures the Dashboard leads
    // with, so the report and the app's own overview always agree.
    report += "<table width='100%' cellpadding='0' cellspacing='0' style='margin: 16px 0;'>"
        "<tr>"
        + kpiCard(money(totalDonations) + " " + L::t("грн"), L::t("Зібрано всього"), p.accent, p)
        + kpiCard(QString::number(donations.size()), L::t("Пожертв"), p.green, p)
        + "</tr><tr>"
        + kpiCard(QString::number(donors.size()), L::t("Донорів"), p.yellow, p)
        + kpiCard(QString("%1 / %2").arg(activeProjects).arg(projects.size()), L::t("Проекти"), p.accent, p)
        + "</tr></table>";

    report += QString("<h3 style='color: %1;'>%2</h3>").arg(p.heading, L::t("Деталі"));
    report += tableOpen(p)
        + QString("<tr style='background-color: %1;'><td><b>%2</b></td><td style='text-align: right;'>%3 %4</td></tr>")
              .arg(p.rowA, L::t("Середня пожертва:"), money(avgDonation), L::t("грн"))
        + QString("<tr><td><b>%1</b></td><td style='text-align: right;'>%2 %3</td></tr>")
              .arg(L::t("Загальна ціль:"), money(totalGoal), L::t("грн"))
        + QString("<tr style='background-color: %1;'><td><b>%2</b></td><td style='text-align: right;'>%3 %4</td></tr>")
              .arg(p.rowA, L::t("Зібрано для проектів:"), money(totalRaised), L::t("грн"))
        + QString("<tr><td><b>%1</b></td><td style='text-align: right;'>%2%</td></tr>")
              .arg(L::t("Прогрес:")).arg(projectProgress, 0, 'f', 1)
        + "</table>";

    report += reportFooter(p);
    return report;
}

QString ReportsWidget::generateDonorStatistics() {
    std::vector<DonorStat> stats = repository_->getDonorStatistics();
    ReportPalette p = ReportPalette::current();

    QString report = reportLetterhead(L::t("Статистика по донорам"), p);
    report += QString("<p style='color: %1; margin-top: 14px;'>%2 %3</p>")
        .arg(p.muted, L::t("Всього донорів:")).arg(stats.size());

    report += tableOpen(p)
        + QString("<tr style='background-color: %1; color: %2;'>"
                   "<th>ID</th><th>%3</th><th>%4</th><th>%5</th><th>%6</th></tr>")
              .arg(p.headerBg, p.heading, L::t("Ім'я"), L::t("Всього пожертв"),
                   L::t("Загальна сума (грн)"), L::t("Остання пожертва"));

    int rowNum = 0;
    for (const auto& stat : stats) {
        QString rowColor = (rowNum % 2 == 0) ? p.rowA : p.rowB;
        report += QString(
            "<tr style='background-color: %1;'>"
            "<td>%2</td><td>%3</td><td style='text-align: center;'>%4</td><td style='text-align: right;'>%5</td><td>%6</td>"
            "</tr>"
        ).arg(rowColor)
         .arg(stat.donorId)
         .arg(stat.donorName)
         .arg(stat.totalDonations)
         .arg(money(stat.totalAmount))
         .arg(stat.lastDonationDate.isEmpty() ? "—" : stat.lastDonationDate);

        rowNum++;
    }

    report += "</table>";
    report += reportFooter(p);
    return report;
}

QString ReportsWidget::generateProjectStatistics() {
    std::vector<Project> projects = repository_->getAllProjects();
    ReportPalette p = ReportPalette::current();

    QString report = reportLetterhead(L::t("Статистика по проектам"), p);
    report += QString("<p style='color: %1; margin-top: 14px;'>%2 %3</p>")
        .arg(p.muted, L::t("Всього проектів:")).arg(projects.size());

    report += tableOpen(p)
        + QString("<tr style='background-color: %1; color: %2;'>"
                   "<th>ID</th><th>%3</th><th>%4</th><th>%5</th><th>%6</th><th>%7</th></tr>")
              .arg(p.headerBg, p.heading, L::t("Назва"), L::t("Ціль (грн)"),
                   L::t("Зібрано (грн)"), L::t("Прогрес"), L::t("Статус"));

    int rowNum = 0;
    for (const auto& project : projects) {
        QString rowColor = (rowNum % 2 == 0) ? p.rowA : p.rowB;
        double progress = project.getProgressPercentage();

        QString progressColor;
        if (progress >= 100) progressColor = p.green;
        else if (progress >= 50) progressColor = p.yellow;
        else progressColor = p.lowProgress;

        report += QString(
            "<tr style='background-color: %1;'>"
            "<td>%2</td><td>%3</td><td style='text-align: right;'>%4</td><td style='text-align: right;'>%5</td>"
            "<td style='text-align: center; color: %6; font-weight: bold;'>%7%</td><td>%8</td>"
            "</tr>"
        ).arg(rowColor)
         .arg(project.getId())
         .arg(QString::fromStdString(project.getName()))
         .arg(money(project.getGoalAmount()))
         .arg(money(project.getCurrentAmount()))
         .arg(progressColor)
         .arg(progress, 0, 'f', 1)
         .arg(L::projectStatusLabel(QString::fromStdString(project.getStatus())));

        rowNum++;
    }

    report += "</table>";
    report += reportFooter(p);
    return report;
}

void ReportsWidget::exportReport() {
    QString fileName = QFileDialog::getSaveFileName(this, L::t("Зберегти звіт"),
        QDir::homePath() + "/report_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".pdf",
        L::t("PDF файли (*.pdf);;HTML файли (*.html);;Текстові файли (*.txt)"));

    if (fileName.isEmpty()) return;

    // Professional reporting software's primary export target is a
    // print-ready PDF, not raw markup — render the same document the
    // on-screen preview shows straight to a paginated PDF via QPrinter.
    if (fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        reportView_->document()->print(&printer);
        QMessageBox::information(this, L::t("Успіх"), L::t("Звіт збережено:\n") + fileName);
        return;
    }

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);

        // reportView_ already holds the translated (L::html) report, so the
        // exported file is the same localized content.
        QString fullHtml = QString(
            "<!DOCTYPE html><html><head>"
            "<meta charset='UTF-8'>"
            "<title>%1</title>"
            "</head><body style='background-color:#ffffff; color:#191817;'>%2</body></html>"
        ).arg(L::t("Звіт - Charity Fund")).arg(reportView_->toHtml());

        out << fullHtml;
        file.close();
        QMessageBox::information(this, L::t("Успіх"), L::t("Звіт збережено:\n") + fileName);
    } else {
        QMessageBox::critical(this, L::t("Помилка"), L::t("Не вдалося зберегти файл!"));
    }
}

void ReportsWidget::refresh() {
    generateReport();
}
