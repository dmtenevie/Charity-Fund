#ifndef LANG_H
#define LANG_H

#include <QString>

// Lightweight UA/EN switcher. Not QTranslator/.ts — a plain dictionary,
// because every source string in this project is already Ukrainian
// literals scattered across the widgets, not tr() calls.
namespace L {

enum Code { Uk, En };

Code code();
bool isUk();
void setCode(Code c);
void toggle();

// Single UI string (label, button text, table header, ...): exact
// dictionary lookup in English mode, Ukrainian text returned unchanged
// otherwise. Falls back to html() if no exact/trimmed match is found,
// so it's safe to call on short compound strings too.
QString t(const char* uk);
QString t(const QString& uk);

// A fully-composed block (generated report, HTML dialog text, multi-
// sentence message) already in Ukrainian: replaces every known phrase
// found inside it, longest keys first, so "Проект" doesn't clobber
// "Проекти" before it gets a chance to match as a whole word.
QString html(const QString& uk);

// The DB stores payment method / project status as fixed English codes
// (cash/bank_transfer/card/online, active/completed) — display-only
// helpers that map a code to a localized label (matching the reference
// design's badges) rather than showing the raw code in tables/combos.
QString paymentMethodLabel(const QString& code);
QString projectStatusLabel(const QString& code);

} // namespace L

#endif // LANG_H
