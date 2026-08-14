#include "gui/MainWindow.h"
#include "gui/AppIcon.h"
#include "database/Database.h"
#include "Theme.h"
#include "Lang.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QIcon appIcon = makeGlyphIcon(QString::fromUtf8("❤"), QColor(Theme::isDark() ? "#74a874" : "#5c8a5c"));
    app.setWindowIcon(appIcon);

    Theme::apply();

    Database& db = Database::getInstance();
    if (!db.connect()) {
        QMessageBox::warning(nullptr, L::t("Помилка підключення"),
            L::t("Не вдалося підключитися до бази даних!\n\n"
                    "Перевірте:\n"
                    "1. PostgreSQL запущений\n"
                    "2. База даних 'charity_fund' створена\n"
                    "3. Користувач 'charity_user' має права\n\n"
                    "Помилка: %1").arg(db.getLastError()));
        // Don't abort: let the UI open so the redesign is visible even
        // without a live database. Tables simply stay empty.
    }

    MainWindow window;
    window.show();

    int result = app.exec();
    db.disconnect();
    
    return result;
}
