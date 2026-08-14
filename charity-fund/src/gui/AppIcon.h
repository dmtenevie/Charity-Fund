#ifndef APPICON_H
#define APPICON_H

#include <QIcon>
#include <QColor>
#include <QString>

// Renders a small circular glyph icon entirely with QPainter, so the app
// doesn't depend on bundled image assets that could go missing from a
// windeployqt6 deployment.
QIcon makeGlyphIcon(const QString& glyph, const QColor& background, int size = 64);

// Custom title-bar control glyphs (minimize/maximize/restore/close/overflow),
// drawn as straight vector lines instead of relying on Unicode box-drawing
// characters ("─", "□", "❐", "✕", "⋯") — those render inconsistently (off-
// center, uneven stroke weight, sometimes missing) across fonts, which is
// what made the frameless window's custom titlebar buttons look crooked.
enum class WinGlyph { Minimize, Maximize, Restore, Close, Overflow };
QIcon makeWindowIcon(WinGlyph glyph, const QColor& color, int size = 16);

#endif // APPICON_H
