#include "eyes.h"
#include <Arduino.h>
#include <math.h>
#include "../logic/weather_mood.h"

static LGFX_Sprite s_spr;
static bool s_ready = false;

static constexpr int kEyeDx = 70;   // Abstand der Augenmitten von der Bildmitte
static constexpr int kEyeY = 100;

bool eyes_begin(LGFX &lcd) {
  if (s_ready) return true;
  s_spr.setPsram(false);
  s_spr.setColorDepth(8);   // RGB332
  s_ready = s_spr.createSprite(EYES_W, EYES_H) != nullptr;
  if (!s_ready) Serial.printf("eyes: kein Speicher fuer Sprite (frei %u), zeichne direkt\n", (unsigned)ESP.getFreeHeap());
  (void)lcd;
  return s_ready;
}

void eyes_free() {
  if (!s_ready) return;
  s_spr.deleteSprite();
  s_ready = false;
}

bool eyes_ready() { return s_ready; }

static uint32_t rgb(Rgb c) { return lgfx::color888(c.r, c.g, c.b); }

static Rgb mix(Rgb a, Rgb b, float k) {
  return Rgb{(uint8_t)(a.r + (b.r - a.r) * k), (uint8_t)(a.g + (b.g - a.g) * k), (uint8_t)(a.b + (b.b - a.b) * k)};
}

// Ein Auge. mirror = linkes Auge (äußere Ecke links).
static void draw_eye(LovyanGFX &d, int cx, int cy, const EyeShape &e, uint32_t col, bool mirror, float sc) {
  const float w = e.w * sc, h = e.h * sc;
  float hh = h * e.open;
  if (hh < 3) hh = 3;
  const int x0 = (int)(cx - w / 2), top = (int)(cy - hh / 2);
  const int iw = (int)w, ih = (int)hh;
  float r = e.radius * sc;
  if (r > hh / 2) r = hh / 2;
  if (r > w / 2) r = w / 2;
  d.fillRoundRect(x0, top, iw, ih, (int)r, col);

  // Oberes Lid: schwarzes Viereck, deckt lid_top_cut*h von oben ab, geneigt um lid_top_tilt*h*0.5.
  const float cut = e.lid_top_cut * hh, tilt = e.lid_top_tilt * h * 0.5f;
  if (cut > 0.5f || fabsf(tilt) > 0.5f) {
    const int xi = mirror ? x0 + iw + 1 : x0 - 1;   // innere Ecke (zur Nase)
    const int xo = mirror ? x0 - 1 : x0 + iw + 1;   // äußere Ecke
    const int yi = (int)(top + cut - tilt / 2);
    const int yo = (int)(top + cut + tilt / 2);
    const int yt = top - 2;
    d.fillTriangle(xi, yt, xo, yt, xo, yo, TFT_BLACK);
    d.fillTriangle(xi, yt, xo, yo, xi, yi, TFT_BLACK);
  }
  // Unteres Lid („^^“-Lächeln): schwarze Ellipse von unten, deckt lid_bottom_cut*h ab.
  if (e.lid_bottom_cut > 0.01f) {
    const int rx = (int)(w * 0.85f), ry = (int)(w * 0.55f);
    const int c = (int)(e.lid_bottom_cut * hh);
    d.fillEllipse(cx, top + ih + ry - c, rx, ry, TFT_BLACK);
  }
}

void eyes_draw_pair(LovyanGFX &d, int cx, int cy, const EyeShape &shape, Rgb color, float scale) {
  const uint32_t col = rgb(color);
  const int dx = (int)(kEyeDx * scale);
  const int gx = (int)(shape.gaze_x * scale), gy = (int)(shape.gaze_y * scale);
  draw_eye(d, cx - dx + gx, cy + gy, shape, col, true, scale);
  draw_eye(d, cx + dx + gx, cy + gy, shape, col, false, scale);
}

static void heart(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 4;
  d.fillCircle(cx - r, cy - r / 2, r, col);
  d.fillCircle(cx + r, cy - r / 2, r, col);
  d.fillTriangle(cx - 2 * r, cy - r / 4, cx + 2 * r, cy - r / 4, cx, cy + s / 2, col);
}

static void drop(LovyanGFX &d, int cx, int cy, int s, uint32_t col) {
  const int r = s / 3;
  d.fillCircle(cx, cy + r, r, col);
  d.fillTriangle(cx - r, cy + r, cx + r, cy + r, cx, cy - s / 2, col);
}

static void letter_z(LovyanGFX &d, int x, int y, int s, uint32_t col) {
  const float t = s / 8.0f + 0.5f;
  d.drawWideLine(x, y, x + s, y, t, col);
  d.drawWideLine(x + s, y, x, y + s, t, col);
  d.drawWideLine(x, y + s, x + s, y + s, t, col);
}

static void render(LovyanGFX &d, const EyeShape &shape, Rgb color, uint32_t ov, uint32_t now) {
  d.fillRect(0, 0, EYES_W, EYES_H, TFT_BLACK);
  Rgb c = color;
  if (ov & OV_RAIN_TINT) c = mix(c, Rgb{60, 120, 255}, 0.3f);
  const int shiver = (ov & OV_SHIVER) ? (((now / 40) & 1) ? 2 : -2) : 0;
  const int cx = EYES_W / 2 + shiver;
  eyes_draw_pair(d, cx, kEyeY, shape, c, 1.0f);

  const int lx = cx - kEyeDx + (int)shape.gaze_x, rx = cx + kEyeDx + (int)shape.gaze_x;
  const int ey = kEyeY + (int)shape.gaze_y;
  const int top = ey - (int)(shape.h * shape.open / 2);

  if (ov & OV_SUNGLASSES) {
    const uint32_t lens = lgfx::color888(40, 40, 48), frame = lgfx::color888(120, 120, 130);
    const int lw = (int)shape.w + 16, lh = (int)(shape.h * 0.62f);
    const int ly = ey - (int)(shape.h / 2) - 6;
    for (int x : {lx, rx}) {
      d.fillRoundRect(x - lw / 2, ly, lw, lh, 10, lens);
      d.drawRoundRect(x - lw / 2, ly, lw, lh, 10, frame);
      d.drawRoundRect(x - lw / 2 + 1, ly + 1, lw - 2, lh - 2, 9, frame);
    }
    d.drawWideLine(lx + lw / 2, ly + 8, rx - lw / 2, ly + 8, 2.0f, frame);
  }
  if (ov & OV_HEART) {
    const int s = 18 + (int)(4 * sinf(now / 200.0f));
    const uint32_t pink = lgfx::color888(255, 80, 170);
    heart(d, lx, top - 22, s, pink);
    heart(d, rx, top - 22, s, pink);
  }
  if (ov & OV_SWEAT) {
    const float ph = (now % 2000) / 2000.0f;
    drop(d, rx + (int)shape.w / 2 + 10, 25 + (int)(ph * 40), 14, lgfx::color888(140, 210, 255));
  }
  if (ov & OV_TEAR) {
    const float ph = (now % 3000) / 3000.0f;
    drop(d, lx - (int)shape.w / 4, ey + (int)(shape.h * shape.open / 2) + 6 + (int)(ph * 30), 12, lgfx::color888(100, 170, 255));
  }
  if (ov & OV_ZZZ) {
    const uint32_t zc = lgfx::color888(150, 170, 255);
    for (int i = 0; i < 3; i++) {
      const float ph = fmodf(now / 3000.0f + i / 3.0f, 1.0f);   // jedes Z steigt in 3 s auf
      const int s = 10 + (int)(ph * 14);
      letter_z(d, 250 + (int)(ph * 30), 80 - (int)(ph * 70), s, zc);
    }
  }
}

void eyes_draw(LGFX &lcd, const EyeShape &shape, Rgb color, uint32_t overlays, uint32_t now_ms) {
  if (s_ready) {
    render(s_spr, shape, color, overlays, now_ms);
    s_spr.pushSprite(&lcd, 0, 0);
  } else {
    lcd.startWrite();
    render(lcd, shape, color, overlays, now_ms);
    lcd.endWrite();
  }
}
