#include "ui_common.h"
#include "fonts.h"

void ui_text(LovyanGFX &d, const char *utf8, int x, int y, const lgfx::IFont *font, uint32_t col, textdatum_t datum) {
  d.setFont(font);
  d.setTextDatum(datum);
  d.setTextColor(col);
  d.drawString(utf8 ? utf8 : "", x, y);
}

void ui_tile(LovyanGFX &d, const UiRect &r, uint32_t fill) {
  d.fillRoundRect(r.x, r.y, r.w, r.h, 14, fill);
  d.drawRoundRect(r.x, r.y, r.w, r.h, 14, TFT_WHITE);
}
