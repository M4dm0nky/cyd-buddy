#include "icons.h"
#include <math.h>

static constexpr float kPi = 3.14159265f;

void icon_play(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s / 2, w = s * 7 / 16;
  d.fillTriangle(cx - w + s / 10, cy - h, cx - w + s / 10, cy + h, cx + w + s / 10, cy, col);
}

void icon_clock(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 2, t = s / 12 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.drawWideLine(cx, cy, cx, cy - r * 6 / 10, t * 0.6f, col);       // Minutenzeiger
  d.drawWideLine(cx, cy, cx + r * 45 / 100, cy, t * 0.6f, col);     // Stundenzeiger
  d.fillCircle(cx, cy, t, col);
}

void icon_gear(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s * 34 / 100, hole = s * 14 / 100;
  const float tooth = s * 0.09f;
  for (int i = 0; i < 8; i++) {
    const float a = i * kPi / 4;
    d.drawWideLine(cx + (int)(cosf(a) * r * 0.6f), cy + (int)(sinf(a) * r * 0.6f),
                   cx + (int)(cosf(a) * s * 0.48f), cy + (int)(sinf(a) * s * 0.48f), tooth, col);
  }
  d.fillArc(cx, cy, hole, r, 0, 360, col);   // Ring statt Loch: Hintergrundfarbe egal
}

void icon_close(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s * 36 / 100;
  const float t = s / 14.0f + 1;
  d.drawWideLine(cx - h, cy - h, cx + h, cy + h, t, col);
  d.drawWideLine(cx - h, cy + h, cx + h, cy - h, t, col);
}

void icon_wifi(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int base = cy + s * 4 / 10, step = s * 26 / 100, t = s / 10 + 1;
  for (int i = 1; i <= 3; i++) {
    const int r = step * i;
    d.fillArc(cx, base, r - t, r, 225, 315, col);
  }
  d.fillCircle(cx, base - t / 2, t, col);
}

void icon_sun(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s * 22 / 100;
  d.fillCircle(cx, cy, r, col);
  for (int i = 0; i < 8; i++) {
    const float a = i * kPi / 4;
    d.drawWideLine(cx + (int)(cosf(a) * r * 1.45f), cy + (int)(sinf(a) * r * 1.45f),
                   cx + (int)(cosf(a) * s * 0.48f), cy + (int)(sinf(a) * s * 0.48f), s / 20.0f + 0.5f, col);
  }
}

// Sichel: gelber Kreis, schwarzer Kreis versetzt darüber (Symbole stehen immer auf schwarzem Grund).
static void moon(LovyanGFX &d, int cx, int cy, int s) {
  const int r = s * 36 / 100;
  d.fillCircle(cx, cy, r, lgfx::color888(240, 230, 150));
  d.fillCircle(cx + r * 55 / 100, cy - r * 25 / 100, r * 85 / 100, TFT_BLACK);
}

static void cloud(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 6;
  d.fillCircle(cx - r, cy, r, col);
  d.fillCircle(cx + r / 2, cy - r / 2, r * 13 / 10, col);
  d.fillCircle(cx + r * 3 / 2, cy + r / 4, r * 3 / 4, col);
  d.fillRoundRect(cx - r * 2, cy, r * 4 + r / 4, r, r / 2, col);
}

void icon_sky(LovyanGFX &d, int cx, int cy, int s, Sky sky, bool is_day) {
  const uint32_t grey = lgfx::color888(170, 180, 195), dark = lgfx::color888(110, 115, 130);
  switch (sky) {
    case Sky::SUNNY:
      if (is_day) icon_sun(d, cx, cy, s, lgfx::color888(255, 210, 0));
      else moon(d, cx, cy, s);
      break;
    case Sky::CLOUDY:
      if (is_day) icon_sun(d, cx + s / 5, cy - s / 5, s * 6 / 10, lgfx::color888(255, 210, 0));
      cloud(d, cx, cy + s / 10, s, grey);
      break;
    case Sky::FOG:
      for (int i = 0; i < 3; i++)
        d.drawWideLine(cx - s * 4 / 10 + (i & 1) * s / 10, cy - s / 4 + i * s / 4, cx + s * 4 / 10, cy - s / 4 + i * s / 4,
                       s / 16.0f + 0.5f, grey);
      break;
    case Sky::RAIN:
      cloud(d, cx, cy - s / 8, s, grey);
      for (int i = -1; i <= 1; i++)
        d.drawWideLine(cx + i * s / 5, cy + s / 6, cx + i * s / 5 - s / 12, cy + s * 4 / 10, s / 24.0f + 0.5f,
                       lgfx::color888(80, 150, 255));
      break;
    case Sky::SNOW:
      cloud(d, cx, cy - s / 8, s, grey);
      for (int i = -1; i <= 1; i++) d.fillCircle(cx + i * s / 5, cy + s / 4 + (i & 1) * s / 8, s / 18 + 1, TFT_WHITE);
      break;
    case Sky::STORM:
      cloud(d, cx, cy - s / 8, s, dark);
      d.fillTriangle(cx + s / 12, cy, cx - s / 8, cy + s / 4, cx + s / 30, cy + s / 4, lgfx::color888(255, 220, 0));
      d.fillTriangle(cx - s / 30, cy + s / 5, cx + s / 8, cy + s / 5, cx - s / 12, cy + s / 2, lgfx::color888(255, 220, 0));
      break;
  }
}

void icon_update(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 2, t = s / 12 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.drawWideLine(cx, cy - r * 55 / 100, cx, cy + r * 15 / 100, t * 0.7f, col);
  d.fillTriangle(cx - r * 4 / 10, cy + r * 5 / 100, cx + r * 4 / 10, cy + r * 5 / 100, cx, cy + r * 55 / 100, col);
}

void icon_info(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 2, t = s / 12 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.fillCircle(cx, cy - r * 45 / 100, t, col);
  d.fillRoundRect(cx - t, cy - r * 18 / 100, 2 * t, r * 70 / 100, t / 2, col);
}

void icon_target(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s * 4 / 10, t = s / 14 + 1;
  d.fillArc(cx, cy, r - t, r, 0, 360, col);
  d.fillArc(cx, cy, r / 2 - t, r / 2, 0, 360, col);
  d.drawWideLine(cx - s / 2, cy, cx + s / 2, cy, t / 2.0f, col);
  d.drawWideLine(cx, cy - s / 2, cx, cy + s / 2, t / 2.0f, col);
}

void icon_simon(LovyanGFX &d, int cx, int cy, int s) {
  const int r = s / 2;
  d.fillArc(cx, cy, r / 3, r, 180 + 3, 270 - 3, lgfx::color888(255, 60, 60));
  d.fillArc(cx, cy, r / 3, r, 270 + 3, 360 - 3, lgfx::color888(60, 220, 90));
  d.fillArc(cx, cy, r / 3, r, 90 + 3, 180 - 3, lgfx::color888(70, 120, 255));
  d.fillArc(cx, cy, r / 3, r, 0 + 3, 90 - 3, lgfx::color888(255, 220, 0));
}

void icon_brightness(LovyanGFX &d, int cx, int cy, int s, uint8_t level, uint32_t col) {
  const int bw = s / 5, gap = s / 10;
  const uint32_t off = lgfx::color888(60, 60, 70);
  for (int i = 0; i < 3; i++) {
    const int h = s * (i + 1) / 3;
    const int x = cx - s / 2 + i * (bw + gap) + gap / 2;
    d.fillRect(x, cy + s / 2 - h, bw, h, i <= level ? col : off);
  }
}

void icon_back(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int h = s * 4 / 10;
  const float t = s / 10.0f + 1;
  d.drawWideLine(cx - h, cy, cx + h, cy, t, col);
  d.fillTriangle(cx - h - s / 10, cy, cx - h + s / 4, cy - s / 3, cx - h + s / 4, cy + s / 3, col);
}
