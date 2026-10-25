#include "bubble.h"
#include "../logic/ascii.h"

static constexpr int kTop = 2, kH = 30, kPad = 10, kTail = 8;

void bubble_draw(LovyanGFX &d, const char *utf8) {
  if (!utf8 || !utf8[0]) return;
  char buf[48];
  ascii_de(utf8, buf, sizeof buf);
  d.setFont(&fonts::FreeSansBold9pt7b);
  int w = d.textWidth(buf) + 2 * kPad;
  if (w > 316) w = 316;
  const int x = 160 - w / 2;
  const uint32_t fill = lgfx::color888(235, 235, 240), edge = lgfx::color888(160, 160, 175);
  d.fillRoundRect(x, kTop, w, kH, 10, fill);
  d.drawRoundRect(x, kTop, w, kH, 10, edge);
  d.fillTriangle(152, kTop + kH - 1, 168, kTop + kH - 1, 160, kTop + kH + kTail, fill);   // Zipfel nach unten zum Gesicht
  d.setTextDatum(middle_center);
  d.setTextColor(TFT_BLACK);
  d.drawString(buf, 160, kTop + kH / 2 + 1);
}
