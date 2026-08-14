#include "MainWindow.h"
#include "AppIcon.h"
#include "../Theme.h"
#include "../Lang.h"
#include <QApplication>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMenu>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QAbstractButton>
#include <QKeySequence>
#include <QMetaObject>
#include <QScreen>
#include <QWindow>
#include <QEvent>
#include <QLocale>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#endif

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), repository_(new Repository()) {

    setWindowIcon(makeGlyphIcon(QString::fromUtf8("❤"), Theme::accent(), 64));
    setWindowFlag(Qt::FramelessWindowHint, true);

    // Size to the design's preferred 1360x840, but never larger than the
    // actual screen — on a 1920x1080 display at 150% scaling (a common
    // laptop config) the available *logical* desktop is only ~1280x720,
    // smaller than a hardcoded 1360x840 would need, which left the window
    // partly off-screen on first launch.
    QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    QSize preferred(1360, 840);
    QSize minimum(1100, 640);
    QSize target = preferred.boundedTo(avail.size() - QSize(24, 24));
    resize(target);
    setMinimumSize(minimum.boundedTo(avail.size() - QSize(24, 24)));
    move(avail.center() - QPoint(target.width() / 2, target.height() / 2));
    applyAccentToWindow();

    setupUI();
    setupActions();

    // Lives in the persistent QStatusBar (unlike the central widget, it
    // survives retranslateUi()'s rebuild), so it's created once here rather
    // than in setupUI() — otherwise every language/theme switch would add
    // another label on top of the old one instead of replacing it.
    statusLabel_ = new QLabel(this);
    statusBar()->addWidget(statusLabel_);
    updateStatusBar();

    setWindowTitle(L::t("Система обліку благодійного фонду"));

#ifdef Q_OS_WIN
    // Restore the thin native Aero drop shadow around the borderless window
    // (Windows strips it entirely once WS_CAPTION is gone).
    HWND hwnd = reinterpret_cast<HWND>(winId());
    MARGINS margins{1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
#endif
}

MainWindow::~MainWindow() {
    delete repository_;
}

void MainWindow::applyAccentToWindow() {
    setWindowIcon(makeGlyphIcon(QString::fromUtf8("❤"), Theme::accent(), 64));
}

void MainWindow::setupUI() {
    // ---- Central area: header (top) + [sidebar | content] ----
    QWidget* central = new QWidget(this);
    QVBoxLayout* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    setCentralWidget(central);

    // ===== Header =====
    headerWidget_ = new QWidget();
    headerWidget_->setObjectName("appHeader");
    QHBoxLayout* header = new QHBoxLayout(headerWidget_);
    header->setContentsMargins(16, 10, 16, 10);
    header->setSpacing(10);

    QLabel* brandIcon = new QLabel(QString::fromUtf8("♥"));
    brandIcon->setObjectName("brandIcon");
    brandIcon->setFixedSize(30, 30);
    brandIcon->setAlignment(Qt::AlignCenter);

    QLabel* brand = new QLabel("CHARITY FUND");
    brand->setObjectName("brandTitle");

    QHBoxLayout* brandBox = new QHBoxLayout();
    brandBox->setSpacing(10);
    brandBox->addWidget(brandIcon);
    brandBox->addWidget(brand);
    header->addLayout(brandBox);

    header->addStretch(1);

    // Segmented language + theme switches: two independent rounded-pill
    // pairs (УКР|ENG and Світла|Темна), each pill's *own* first/last button
    // needs the segLeft/segRight radius — using segMid on ENG and Світла
    // left their outer edges square, breaking the pill shape.
    ukButton_ = new QPushButton("УКР");
    ukButton_->setObjectName("segLeft");
    enButton_ = new QPushButton("ENG");
    enButton_->setObjectName("segRight");
    lightButton_ = new QPushButton(L::t("Світла"));
    lightButton_->setObjectName("segLeft");
    darkButton_ = new QPushButton(L::t("Темна"));
    darkButton_->setObjectName("segRight");
    for (auto* b : {ukButton_, enButton_, lightButton_, darkButton_}) {
        b->setCheckable(true);
        b->setFixedHeight(32);
    }
    ukButton_->setChecked(L::isUk());
    enButton_->setChecked(!L::isUk());
    darkButton_->setChecked(Theme::isDark());
    lightButton_->setChecked(!Theme::isDark());

    connect(ukButton_, &QPushButton::clicked, this, [this]() { if (!L::isUk()) toggleLanguage(); });
    connect(enButton_, &QPushButton::clicked, this, [this]() { if (L::isUk()) toggleLanguage(); });
    connect(lightButton_, &QPushButton::clicked, this, [this]() { if (Theme::isDark()) toggleTheme(); });
    connect(darkButton_, &QPushButton::clicked, this, [this]() { if (!Theme::isDark()) toggleTheme(); });

    QHBoxLayout* seg = new QHBoxLayout();
    seg->setSpacing(0);
    seg->addWidget(ukButton_);
    seg->addWidget(enButton_);
    seg->addSpacing(8);
    seg->addWidget(lightButton_);
    seg->addWidget(darkButton_);
    header->addLayout(seg);

    // Overflow menu (About / DB status) + custom window controls — this
    // window is frameless (see the constructor), so these replace the
    // native title bar's system menu and min/max/close buttons entirely.
    // Drawn as vector icons (see AppIcon::makeWindowIcon) rather than
    // Unicode glyphs ("─"/"□"/"✕"/"⋯"), which rendered off-center and with
    // uneven stroke weight depending on font/DPI.
    const QColor winIconColor = Theme::isDark() ? QColor("#a3a29d") : QColor("#6a655e");
    const QSize winIconSize(14, 14);

    menuButton_ = new QPushButton();
    menuButton_->setObjectName("winIconBtn");
    menuButton_->setFixedSize(32, 32);
    menuButton_->setIcon(makeWindowIcon(WinGlyph::Overflow, winIconColor, 16));
    menuButton_->setIconSize(winIconSize);
    connect(menuButton_, &QPushButton::clicked, this, [this]() {
        QMenu menu(this);
        QAction* dbCheck = menu.addAction(L::t("Перевірити БД"));
        QAction* about = menu.addAction(L::t("Про програму"));
        connect(dbCheck, &QAction::triggered, this, &MainWindow::checkDatabaseConnection);
        connect(about, &QAction::triggered, this, &MainWindow::showAbout);
        menu.exec(menuButton_->mapToGlobal(QPoint(0, menuButton_->height())));
    });

    minButton_ = new QPushButton();
    minButton_->setObjectName("winIconBtn");
    minButton_->setFixedSize(32, 32);
    minButton_->setIcon(makeWindowIcon(WinGlyph::Minimize, winIconColor, 16));
    minButton_->setIconSize(winIconSize);
    minButton_->setToolTip(L::t("Згорнути"));
    connect(minButton_, &QPushButton::clicked, this, &QWidget::showMinimized);

    maxButton_ = new QPushButton();
    maxButton_->setObjectName("winIconBtn");
    maxButton_->setFixedSize(32, 32);
    maxButton_->setIcon(makeWindowIcon(isMaximized() ? WinGlyph::Restore : WinGlyph::Maximize, winIconColor, 16));
    maxButton_->setIconSize(winIconSize);
    maxButton_->setToolTip(isMaximized() ? L::t("Відновити") : L::t("Розгорнути"));
    connect(maxButton_, &QPushButton::clicked, this, &MainWindow::toggleMaximizeRestore);

    closeButton_ = new QPushButton();
    closeButton_->setObjectName("winCloseBtn");
    closeButton_->setFixedSize(32, 32);
    closeButton_->setIcon(makeWindowIcon(WinGlyph::Close, winIconColor, 16));
    closeButton_->setIconSize(winIconSize);
    closeButton_->setToolTip(L::t("Закрити"));
    connect(closeButton_, &QPushButton::clicked, this, &QWidget::close);

    QHBoxLayout* winCtrls = new QHBoxLayout();
    winCtrls->setSpacing(2);
    winCtrls->addSpacing(6);
    winCtrls->addWidget(menuButton_);
    winCtrls->addWidget(minButton_);
    winCtrls->addWidget(maxButton_);
    winCtrls->addWidget(closeButton_);
    header->addLayout(winCtrls);

    root->addWidget(headerWidget_);

    // ===== Body: sidebar + content =====
    QHBoxLayout* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    root->addLayout(body, 1);

    // ---- Sidebar ----
    QWidget* sidebar = new QWidget();
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(248);
    QVBoxLayout* sb = new QVBoxLayout(sidebar);
    sb->setContentsMargins(14, 16, 14, 16);
    sb->setSpacing(4);

    // Fund identity
    QLabel* fundName = new QLabel(L::t("Фонд «Милосердя»"));
    fundName->setObjectName("sidebarFund");
    QLabel* fundYear = new QLabel(L::t("Звітний рік 2026"));
    fundYear->setObjectName("sidebarYear");
    sb->addWidget(fundName);
    sb->addWidget(fundYear);
    sb->addSpacing(14);

    // Nav items: Огляд alone, then ОБЛІК (record-keeping tabs with counts),
    // then АНАЛІТИКА (Звіти alone) — matches the design's three sidebar groups.
    // Plain text only (no icons) — the sidebar is text-first by design, unlike
    // the icon buttons in the header.
    struct NavDef { QString label; };
    const NavDef navMain[]  = { {L::t("Огляд")} };
    const NavDef navData[]  = {
        {L::t("Донори")},
        {L::t("Пожертви")},
        {L::t("Проекти")},
        {L::t("Бенефіціари")},
    };
    const NavDef navAnalytics[] = { {L::t("Звіти")} };

    navButtons_.clear();
    navBadges_.clear();
    int navIndex = 0;

    auto makeNavButton = [&](const NavDef& def, int index) {
        QPushButton* b = new QPushButton(def.label);
        b->setObjectName("navItem");
        b->setCheckable(true);
        b->setProperty("navIndex", index);
        b->setFixedHeight(36);
        connect(b, &QPushButton::clicked, this, [this, index]() { navigate(index); });
        navButtons_.append(b);
        return b;
    };

    for (const auto& def : navMain) {
        sb->addWidget(makeNavButton(def, navIndex++));
    }

    sb->addSpacing(14);
    QLabel* accHeader = new QLabel(L::t("ОБЛІК"));
    accHeader->setObjectName("sidebarHeader");
    sb->addWidget(accHeader);

    for (const auto& def : navData) {
        QPushButton* b = makeNavButton(def, navIndex++);
        QLabel* badge = new QLabel("—");
        badge->setObjectName("navBadge");
        badge->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        badge->setMinimumWidth(26);
        QWidget* row = new QWidget();
        QHBoxLayout* rowLay = new QHBoxLayout(row);
        rowLay->setContentsMargins(0, 0, 8, 0);
        rowLay->setSpacing(4);
        rowLay->addWidget(b, 1);
        rowLay->addWidget(badge, 0);
        sb->addWidget(row);
        navBadges_.append(badge);
    }

    sb->addSpacing(14);
    QLabel* anaHeader = new QLabel(L::t("АНАЛІТИКА"));
    anaHeader->setObjectName("sidebarHeader");
    sb->addWidget(anaHeader);

    for (const auto& def : navAnalytics) {
        sb->addWidget(makeNavButton(def, navIndex++));
    }

    if (!navButtons_.isEmpty()) navButtons_.first()->setChecked(true);
    sb->addStretch(1);

    // User profile
    QWidget* profile = new QWidget();
    profile->setObjectName("sidebarProfile");
    Theme::applyCardShadow(profile, 18, 4, 90);
    QHBoxLayout* pf = new QHBoxLayout(profile);
    pf->setContentsMargins(8, 8, 8, 8);
    pf->setSpacing(10);
    QLabel* avatar = new QLabel(L::t("АД"));
    avatar->setObjectName("avatar");
    avatar->setAlignment(Qt::AlignCenter);
    QVBoxLayout* pfText = new QVBoxLayout();
    pfText->setSpacing(2);
    QLabel* uname = new QLabel(L::t("Адміністратор"));
    uname->setObjectName("profileName");
    QLabel* urole = new QLabel(L::t("Керівник фонду"));
    urole->setObjectName("profileRole");
    pfText->addWidget(uname);
    pfText->addWidget(urole);
    pf->addWidget(avatar);
    pf->addLayout(pfText, 1);
    sb->addWidget(profile);

    body->addWidget(sidebar);

    // ---- Content stack ----
    stack_ = new QStackedWidget();
    dashboardWidget_ = new DashboardWidget(repository_, this);
    donorsWidget_ = new DonorsWidget(repository_, this);
    donationsWidget_ = new DonationsWidget(repository_, this);
    projectsWidget_ = new ProjectsWidget(repository_, this);
    beneficiariesWidget_ = new BeneficiariesWidget(repository_, this);
    reportsWidget_ = new ReportsWidget(repository_, this);

    stack_->addWidget(dashboardWidget_);
    stack_->addWidget(donorsWidget_);
    stack_->addWidget(donationsWidget_);
    stack_->addWidget(projectsWidget_);
    stack_->addWidget(beneficiariesWidget_);
    stack_->addWidget(reportsWidget_);
    body->addWidget(stack_, 1);

    updateNavBadges();
}

void MainWindow::navigate(int index) {
    stack_->setCurrentIndex(index);
    for (int i = 0; i < navButtons_.size(); ++i)
        navButtons_[i]->setChecked(i == index);

    // Re-query the destination tab so data edited elsewhere (e.g. a donor
    // added on the Донори tab) is never stale when the user switches back.
    QWidget* page = stack_->widget(index);
    if (page) QMetaObject::invokeMethod(page, "refresh");
    updateNavBadges();
}

void MainWindow::updateNavBadges() {
    if (!repository_ || navBadges_.size() < 4) return;
    navBadges_[0]->setText(QString::number(repository_->getAllDonors().size()));
    navBadges_[1]->setText(QString::number(repository_->getAllDonations().size()));
    navBadges_[2]->setText(QString::number(repository_->getAllProjects().size()));
    navBadges_[3]->setText(QString::number(repository_->getAllBeneficiaries().size()));
}

void MainWindow::setupActions() {
    // No visible menu bar in the frameless design — these exist purely as
    // keyboard shortcuts (the equivalent actions are also reachable via the
    // "⋯" overflow button and the УКР/ENG · Світла/Темна switches).
    QAction* exitAction = new QAction(this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    addAction(exitAction);

    QAction* themeA = new QAction(this);
    themeA->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeA, &QAction::triggered, this, &MainWindow::toggleTheme);
    addAction(themeA);

    QAction* langA = new QAction(this);
    langA->setShortcut(QKeySequence("Ctrl+L"));
    connect(langA, &QAction::triggered, this, &MainWindow::toggleLanguage);
    addAction(langA);
}

void MainWindow::toggleTheme() {
    Theme::setDark(!Theme::isDark());
    Theme::apply();
    darkButton_->setChecked(Theme::isDark());
    lightButton_->setChecked(!Theme::isDark());
    refreshAll();
}

void MainWindow::toggleLanguage() {
    L::toggle();
    retranslateUi();
}

void MainWindow::toggleMaximizeRestore() {
    if (isMaximized())
        showNormal();
    else
        showMaximized();
}

void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && maxButton_) {
        bool maxed = isMaximized();
        const QColor winIconColor = Theme::isDark() ? QColor("#a3a29d") : QColor("#6a655e");
        maxButton_->setIcon(makeWindowIcon(maxed ? WinGlyph::Restore : WinGlyph::Maximize, winIconColor, 16));
        maxButton_->setToolTip(maxed ? L::t("Відновити") : L::t("Розгорнути"));
    }
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(eventType);
    MSG* msg = static_cast<MSG*>(message);

    if (msg->message == WM_NCHITTEST) {
        *result = 0;
        HWND hwnd = reinterpret_cast<HWND>(winId());
        RECT winrect;
        GetWindowRect(hwnd, &winrect);
        const long x = GET_X_LPARAM(msg->lParam);
        const long y = GET_Y_LPARAM(msg->lParam);

        if (!isMaximized()) {
            const long border = 8;
            const bool onLeft = x < winrect.left + border;
            const bool onRight = x >= winrect.right - border;
            const bool onTop = y < winrect.top + border;
            const bool onBottom = y >= winrect.bottom - border;

            if (onTop && onLeft) *result = HTTOPLEFT;
            else if (onTop && onRight) *result = HTTOPRIGHT;
            else if (onBottom && onLeft) *result = HTBOTTOMLEFT;
            else if (onBottom && onRight) *result = HTBOTTOMRIGHT;
            else if (onLeft) *result = HTLEFT;
            else if (onRight) *result = HTRIGHT;
            else if (onTop) *result = HTTOP;
            else if (onBottom) *result = HTBOTTOM;

            if (*result != 0) return true;
        }

        // Not a resize border: drag the window from any empty area of the
        // custom header, but let clicks on its buttons/switches through.
        const qreal dpr = windowHandle() ? windowHandle()->devicePixelRatio() : 1.0;
        const QPoint globalLogical(qRound(x / dpr), qRound(y / dpr));
        const QPoint headerLocal = headerWidget_->mapFromGlobal(globalLogical);
        if (headerWidget_->rect().contains(headerLocal)) {
            QWidget* child = headerWidget_->childAt(headerLocal);
            const bool interactive = child && (qobject_cast<QAbstractButton*>(child) ||
                                                qobject_cast<QComboBox*>(child) ||
                                                qobject_cast<QLineEdit*>(child));
            *result = interactive ? HTCLIENT : HTCAPTION;
        } else {
            *result = HTCLIENT;
        }
        return true;
    }

    if (msg->message == WM_NCLBUTTONDBLCLK && msg->wParam == HTCAPTION) {
        toggleMaximizeRestore();
        *result = 0;
        return true;
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void MainWindow::retranslateUi() {
    setWindowTitle(L::t("Система обліку благодійного фонду"));
    // The whole UI is rebuilt from current language strings on next setup.
    // Simplest reliable approach: tear down and rebuild central widget.
    QWidget* old = takeCentralWidget();
    delete old;
    setupUI();
    setupActions();
    updateStatusBar();
}

void MainWindow::refreshAll() {
    dashboardWidget_->refresh();
    donorsWidget_->refresh();
    donationsWidget_->refresh();
    projectsWidget_->refresh();
    beneficiariesWidget_->refresh();
    reportsWidget_->refresh();
    updateStatusBar();
    updateNavBadges();
}

void MainWindow::updateStatusBar() {
    if (Database::getInstance().isConnected()) {
        double total = repository_->getTotalDonations();
        QLocale ua(QLocale::Ukrainian, QLocale::Ukraine);
        statusLabel_->setText(QString("%1  |  %2: %3 %4")
            .arg(L::t("Підключено до БД"))
            .arg(L::t("Загальна сума пожертв"))
            .arg(ua.toString(total, 'f', 2))
            .arg(L::t("грн")));
    } else {
        statusLabel_->setText(L::t("Немає підключення до БД"));
    }
}

void MainWindow::checkDatabaseConnection() {
    Database& db = Database::getInstance();
    if (db.isConnected()) {
        QMessageBox::information(this, L::t("Статус БД"),
            L::t("Підключення до бази даних активне") + "\n\n" +
            L::t("БД: ") + db.getDatabaseName() + "\n" +
            L::t("Хост: ") + QString("%1:%2").arg(db.getHost()).arg(db.getPort()));
    } else {
        QMessageBox::critical(this, L::t("Помилка БД"),
            L::t("Немає підключення до бази даних") + "\n\n" +
            L::t("Перевірте:\n1. PostgreSQL запущений\n2. База даних 'charity_fund' існує\n"
                 "3. Користувач 'charity_user' має права доступу\n\n") +
            L::t("Помилка: ") + db.getLastError());
    }
}

void MainWindow::showAbout() {
    QMessageBox::about(this, L::t("Про програму"),
        L::html(
        "<h2>Система обліку благодійного фонду</h2>"
        "<p><b>Версія:</b> 1.0</p>"
        "<p><b>Курсова робота</b> з основ програмування</p>"
        "<hr>"
        "<p><b>Функціонал:</b></p>"
        "<ul>"
        "<li>Облік донорів (благодійників)</li>"
        "<li>Реєстрація та облік пожертв</li>"
        "<li>Управління благодійними проектами</li>"
        "<li>Облік бенефіціарів</li>"
        "<li>Генерація звітів</li>"
        "</ul>"
        "<hr>"
        "<p><b>Технології:</b> C++17, Qt6, PostgreSQL</p>"
        "<p>© 2026</p>"));
}
