#include "Theme.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QWidget>
#include <QGraphicsDropShadowEffect>

namespace {
bool g_loaded = false;
bool g_isDark = true;

// Fixed brand accent (a calm sage green). There is no in-app color picker,
// so this is not user-configurable or persisted.
const QColor kAccentColor = QColor("#5c8a5c");

void ensureLoaded() {
    if (g_loaded) return;
    QSettings settings("CharityFund", "CharityFund");
    g_isDark = settings.value("ui/darkTheme", true).toBool();
    g_loaded = true;
}

// Derive the hover/pressed/soft/border variants from the base accent.
QColor accentHover(const QColor& c)  { return c.lighter(112); }
QColor accentPress(const QColor& c)  { return c.darker(118); }
QColor accentSoft(const QColor& c) {
    QColor s = c; s.setAlpha(28); return s;
}
QColor accentBorder(const QColor& c) {
    QColor b = c; b.setAlpha(70); return b;
}
QString toHex(const QColor& c) { return c.name(QColor::HexRgb); }
QString toRgba(const QColor& c) {
    return QString("rgba(%1,%2,%3,%4)")
        .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha());
}
} // namespace

namespace Theme {

bool isDark() {
    ensureLoaded();
    return g_isDark;
}

void setDark(bool dark) {
    ensureLoaded();
    g_isDark = dark;
    QSettings settings("CharityFund", "CharityFund");
    settings.setValue("ui/darkTheme", dark);
}

QColor accent() {
    return kAccentColor;
}

void apply() {
    ensureLoaded();
    QString fileName = isDark() ? ":/styles.qss" : ":/styles_light.qss";
    QFile styleFile(fileName);
    if (!styleFile.open(QFile::ReadOnly | QFile::Text)) return;

    QTextStream stream(&styleFile);
    QString sheet = stream.readAll();
    styleFile.close();

    // Substitute the live accent into the placeholder tokens.
    QColor a = kAccentColor;
    sheet.replace("@ACCENT@",        toHex(a));
    sheet.replace("@ACCENT_HOVER@",  toHex(accentHover(a)));
    sheet.replace("@ACCENT_PRESS@",  toHex(accentPress(a)));
    sheet.replace("@ACCENT_TEXT@",   "#ffffff");
    sheet.replace("@ACCENT_SOFT@",   toRgba(accentSoft(a)));
    sheet.replace("@ACCENT_BORDER@", toRgba(accentBorder(a)));

    qApp->setStyleSheet(sheet);
}

void applyCardShadow(QWidget* widget, int blurRadius, int yOffset, int alpha) {
    if (!widget) return;
    auto* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(0, yOffset);
    shadow->setColor(QColor(0, 0, 0, alpha));
    widget->setGraphicsEffect(shadow);
}

} // namespace Theme
