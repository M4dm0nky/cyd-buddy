#include "ui_common.h"
#include "../logic/ascii.h"

void ui_text(LovyanGFX &d, const char *utf8, int x, int y, const lgfx::IFont *font, uint32_t col, textdatum_t datum) {
  char buf[96];
  ascii_de(utf8, buf, sizeof buf);
  d.setFont(font);
  d.setTextDatum(datum);
  d.setTextColor(col);
  d.drawString(buf, x, y);
}

void ui_tile(LovyanGFX &d, const UiRect &r, uint32_t fill) {
  d.fillRoundRect(r.x, r.y, r.w, r.h, 14, fill);
  d.drawRoundRect(r.x, r.y, r.w, r.h, 14, TFT_WHITE);
}
