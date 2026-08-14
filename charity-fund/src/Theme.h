#ifndef THEME_H
#define THEME_H

#include <QColor>

class QWidget;

// Tracks which of the two bundled base stylesheets (resources/styles.qss —
// dark, resources/styles_light.qss — light) is active, plus the app's fixed
// brand accent, so widgets that can't be themed purely through QSS
// (QChart, per-cell QProgressBar stylesheets, generated report HTML) know
// which palette to draw with.
//
// The accent is substituted into the base stylesheet at runtime (the .qss
// files carry @ACCENT@ / @ACCENT_HOVER@ / @ACCENT_PRESS@ / @ACCENT_TEXT@ /
// @ACCENT_SOFT@ / @ACCENT_BORDER@ placeholders) so the whole UI can be
// re-tinted without recompiling.
namespace Theme {

bool isDark();
void setDark(bool dark);
QColor accent();

// Applies the corresponding bundled stylesheet (with the active accent
// substituted in) to the QApplication.
void apply();

// Attaches a soft drop shadow to a card/panel widget (dashboard metric
// cards, sidebar profile, tables, dialogs) so the flat QSS panels get the
// same subtle depth as the reference design. Safe to call once per widget;
// QWidget owns the effect and destroys it with the widget.
void applyCardShadow(QWidget* widget, int blurRadius = 28, int yOffset = 8, int alpha = 130);

} // namespace Theme

#endif // THEME_H
