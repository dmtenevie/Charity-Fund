#include "DashboardWidget.h"
#include "../Lang.h"
#include "../Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QProgressBar>
#include <QLocale>

namespace {

// A single metric card: small uppercase caption + big value + sub-caption.
QFrame* makeCard(const QString& caption, QLabel** valueOut, QLabel** noteOut = nullptr) {
    QFrame* card = new QFrame();
    card->setObjectName("metricCard");
    card->setFrameShape(QFrame::StyledPanel);
    Theme::applyCardShadow(card);

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(20, 18, 20, 18);
    lay->setSpacing(8);

    QLabel* captionLabel = new QLabel(caption.toUpper());
    captionLabel->setObjectName("metricCaption");

    QLabel* value = new QLabel("—");
    value->setObjectName("metricValue");

    lay->addWidget(captionLabel);
    lay->addWidget(value);

    if (noteOut) {
        QLabel* note = new QLabel();
        note->setObjectName("projectValue");
        lay->addWidget(note);
        *noteOut = note;
    }

    lay->addStretch(1);

    *valueOut = value;
    return card;
}

QString money(double v) {
    QLocale ua(QLocale::Ukrainian, QLocale::Ukraine);
    return ua.toString(v, 'f', 2) + " ₴";
}

// Recursively removes and deletes everything a layout owns — both direct
// widgets and nested child layouts (with their own widgets). Rows in this
// file are built as nested QHBoxLayout/QVBoxLayout trees, so a shallow
// `delete item->widget()` leaves those nested widgets parented but
// unmanaged: they keep their last on-screen position and get drawn
// underneath whatever the next refresh() adds, producing overlapping text.
void clearLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            delete w;
        } else if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}

} // namespace

DashboardWidget::DashboardWidget(Repository* repository, QWidget* parent)
    : QWidget(parent), repository_(repository) {

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(20);

    // Heading
    QVBoxLayout* head = new QVBoxLayout();
    head->setSpacing(4);
    QLabel* title = new QLabel(L::t("Огляд фонду"));
    title->setObjectName("pageTitle");
    QLabel* sub = new QLabel(L::t("Ключові показники за поточний звітний період"));
    sub->setObjectName("pageSubtitle");
    head->addWidget(title);
    head->addWidget(sub);
    root->addLayout(head);

    // Metric cards row
    QHBoxLayout* cards = new QHBoxLayout();
    cards->setSpacing(16);
    QWidget* c1 = makeCard(L::t("Зібрано всього"), &totalLabel_);
    QWidget* c2 = makeCard(L::t("Пожертв"), &donationsLabel_, &donationsNoteLabel_);
    QWidget* c3 = makeCard(L::t("Донорів"), &donorsLabel_);
    QWidget* c4 = makeCard(L::t("Проекти"), &projectsLabel_, &projectsNoteLabel_);
    cards->addWidget(c1);
    cards->addWidget(c2);
    cards->addWidget(c3);
    cards->addWidget(c4);
    root->addLayout(cards, 0);

    // Body: project progress (left, wider) + recent donations (right).
    QHBoxLayout* bodyRow = new QHBoxLayout();
    bodyRow->setSpacing(16);

    QFrame* projCard = new QFrame();
    projCard->setObjectName("metricCard");
    Theme::applyCardShadow(projCard);
    QVBoxLayout* pl = new QVBoxLayout(projCard);
    pl->setContentsMargins(20, 18, 20, 18);
    pl->setSpacing(14);

    QLabel* projTitle = new QLabel(L::t("Виконання проектів"));
    projTitle->setObjectName("cardTitle");
    pl->addWidget(projTitle);

    projectList_ = new QWidget();
    QVBoxLayout* listLay = new QVBoxLayout(projectList_);
    listLay->setContentsMargins(0, 0, 0, 0);
    listLay->setSpacing(14);
    pl->addWidget(projectList_);
    bodyRow->addWidget(projCard, 2);

    QFrame* recentCard = new QFrame();
    recentCard->setObjectName("metricCard");
    Theme::applyCardShadow(recentCard);
    QVBoxLayout* rl = new QVBoxLayout(recentCard);
    rl->setContentsMargins(20, 18, 20, 18);
    rl->setSpacing(10);

    QLabel* recentTitle = new QLabel(L::t("Останні пожертви"));
    recentTitle->setObjectName("cardTitle");
    rl->addWidget(recentTitle);

    recentList_ = new QWidget();
    QVBoxLayout* recentLay = new QVBoxLayout(recentList_);
    recentLay->setContentsMargins(0, 0, 0, 0);
    recentLay->setSpacing(10);
    rl->addWidget(recentList_);
    bodyRow->addWidget(recentCard, 1);

    root->addLayout(bodyRow, 1);

    refresh();
}

void DashboardWidget::refresh() {
    if (!repository_) return;

    double total = repository_->getTotalDonations();
    auto donors = repository_->getAllDonors();
    auto donations = repository_->getAllDonations();
    auto projects = repository_->getAllProjects();

    totalLabel_->setText(money(total));
    donationsLabel_->setText(QString::number(donations.size()));
    donationsNoteLabel_->setText(donations.empty() ? QString()
        : QString("%1 %2").arg(L::t("середня")).arg(money(total / donations.size())));
    donorsLabel_->setText(QString::number(donors.size()));

    int active = 0, completed = 0;
    for (const auto& p : projects) {
        if (p.getStatus() == "active") ++active; else ++completed;
    }
    projectsLabel_->setText(QString("%1 / %2 %3").arg(active).arg(projects.size()).arg(L::t("активні")));
    projectsNoteLabel_->setText(QString("%1 %2").arg(completed).arg(L::t("завершено")));

    // Rebuild the recent-donations list (most recent first; the underlying
    // view is already sorted by date).
    clearLayout(recentList_->layout());
    auto recentDonations = repository_->getAllDonationsWithNames();
    int shown = 0;
    for (const auto& d : recentDonations) {
        if (shown++ >= 5) break;
        QHBoxLayout* row = new QHBoxLayout();
        row->setSpacing(8);

        QVBoxLayout* left = new QVBoxLayout();
        left->setSpacing(2);
        QLabel* donor = new QLabel(d.donorName);
        donor->setObjectName("projectName");
        QLabel* project = new QLabel(d.projectName);
        project->setObjectName("projectValue");
        left->addWidget(donor);
        left->addWidget(project);

        QVBoxLayout* right = new QVBoxLayout();
        right->setSpacing(2);
        QLabel* amount = new QLabel(money(d.amount));
        amount->setObjectName("projectName");
        amount->setAlignment(Qt::AlignRight);
        QLabel* date = new QLabel(d.date);
        date->setObjectName("projectValue");
        date->setAlignment(Qt::AlignRight);
        right->addWidget(amount);
        right->addWidget(date);

        row->addLayout(left, 1);
        row->addLayout(right, 0);
        // addLayout() (not the base addItem()) so the row's child widgets get
        // properly reparented into recentList_'s widget tree — addItem() on a
        // raw nested QLayout skips that and leaves the labels unparented (and
        // therefore invisible).
        static_cast<QVBoxLayout*>(recentList_->layout())->addLayout(row);
    }
    if (recentDonations.empty()) {
        QLabel* empty = new QLabel(L::t("Немає даних"));
        empty->setObjectName("projectValue");
        recentList_->layout()->addWidget(empty);
    }

    // Rebuild the project progress rows.
    clearLayout(projectList_->layout());
    projectBars_.clear();

    for (const auto& p : projects) {
        double goal = p.getGoalAmount();
        double cur = p.getCurrentAmount();
        int pct = goal > 0 ? static_cast<int>(cur / goal * 100.0) : 0;

        QVBoxLayout* row = new QVBoxLayout();
        row->setSpacing(6);

        QHBoxLayout* names = new QHBoxLayout();
        QLabel* name = new QLabel(QString::fromStdString(p.getName()));
        name->setObjectName("projectName");
        QLabel* vals = new QLabel(QString("%1 / %2   ·   %3%")
            .arg(money(cur)).arg(money(goal)).arg(pct));
        vals->setObjectName("projectValue");
        names->addWidget(name);
        names->addStretch(1);
        names->addWidget(vals);

        QProgressBar* bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setValue(pct);
        bar->setTextVisible(false);
        bar->setFixedHeight(8);

        row->addLayout(names);
        row->addWidget(bar);
        // See the matching comment above: addLayout(), not addItem(), so the
        // row's labels/progress bar are actually parented and shown.
        static_cast<QVBoxLayout*>(projectList_->layout())->addLayout(row);
        projectBars_.append(bar);
    }
}
