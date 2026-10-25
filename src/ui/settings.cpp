#include "settings.h"
#include <Arduino.h>
#include <stdio.h>
#include "icons.h"
#include "ui_common.h"
#include "../hw/light.h"
#include "../net/net.h"

static constexpr uint32_t kTimeoutMs = 30000;
static constexpr int kRowH = 52, kRowY0 = 32;
static constexpr UiRect kClose = {268, 0, 52, 44};
static UiRect row(int i) { return UiRect{0, (int16_t)(kRowY0 + i * kRowH), 320, kRowH}; }

static uint32_t s_last_input = 0, s_last_tick = 0;
static int (*s_fw)(char *, size_t, uint32_t) = nullptr;

void settings_firmware_source(int (*fn)(char *out, size_t n, uint32_t now_ms)) { s_fw = fn; }

static void row_bg(LGFX &d, int i) {
  const UiRect r = row(i);
  d.fillRect(r.x, r.y, r.w, r.h, TFT_BLACK);
  d.drawFastHLine(10, r.y + r.h - 1, 300, d.color888(40, 50, 70));
}

static void draw_brightness(LGFX &d, const Persist &p) {
  row_bg(d, 0);
  const int cy = row(0).y + kRowH / 2;
  icon_sun(d, 30, cy, 34, d.color888(255, 210, 0));
  icon_brightness(d, 100, cy, 36, p.brightness, d.color888(255, 210, 0));
}

static void draw_wifi(LGFX &d, const Persist &p) {
  row_bg(d, 1);
  const int cy = row(1).y + kRowH / 2;
  const uint32_t col = net_wifi_ok() ? d.color888(60, 220, 120) : d.color888(255, 60, 60);
  icon_wifi(d, 30, cy, 30, col);
  const char *txt = net_portal_active() ? "Portal offen: CYD-Buddy" : (p.place[0] ? p.place : "WLAN / Ort einrichten");
  ui_text(d, txt, 62, cy, &fonts::FreeSans9pt7b, TFT_WHITE, middle_left);
}

static void draw_firmware(LGFX &d, uint32_t now_ms) {
  row_bg(d, 2);
  const int cy = row(2).y + kRowH / 2;
  char buf[48] = "\xE2\x80\x93";   // „–“ bis OTA da ist
  int tone = 0;
  if (s_fw) tone = s_fw(buf, sizeof buf, now_ms);
  const uint32_t col = tone == 1 ? d.color888(60, 220, 120) : (tone == 2 ? d.color888(255, 80, 80) : d.color888(200, 200, 210));
  icon_update(d, 30, cy, 30, d.color888(80, 200, 255));
  ui_text(d, buf, 62, cy, &fonts::FreeSans9pt7b, col, middle_left);
}

static void draw_info(LGFX &d) {
  row_bg(d, 3);
  const int cy = row(3).y + kRowH / 2;
  icon_info(d, 30, cy, 30, d.color888(180, 180, 255));
  char buf[64];
  snprintf(buf, sizeof buf, "%s  Heap %u", __DATE__, (unsigned)ESP.getFreeHeap());
  ui_text(d, buf, 62, cy - 9, &fonts::Font2, d.color888(200, 200, 210), middle_left);
  snprintf(buf, sizeof buf, "Licht %d", light_raw());
  ui_text(d, buf, 62, cy + 9, &fonts::Font2, d.color888(200, 200, 210), middle_left);
}

void settings_redraw(LGFX &lcd, const Persist &p) {
  lcd.startWrite();
  draw_brightness(lcd, p);
  draw_wifi(lcd, p);
  draw_firmware(lcd, millis());
  draw_info(lcd);
  lcd.endWrite();
}

void settings_open(LGFX &lcd, const Persist &p, uint32_t now_ms) {
  s_last_input = s_last_tick = now_ms;
  lcd.startWrite();
  lcd.fillScreen(TFT_BLACK);
  icon_gear(lcd, 20, 16, 24, lcd.color888(170, 110, 255));
  icon_close(lcd, kClose.x + kClose.w / 2, kClose.y + kClose.h / 2 - 4, 28, lcd.color888(255, 70, 70));
  lcd.endWrite();
  settings_redraw(lcd, p);
}

SettingsAction settings_touch(int16_t x, int16_t y, uint32_t now_ms) {
  s_last_input = now_ms;
  if (ui_hit(kClose, x, y)) return SettingsAction::CLOSE;
  if (ui_hit(row(0), x, y)) return SettingsAction::BRIGHTNESS;
  if (ui_hit(row(1), x, y)) return SettingsAction::PORTAL;
  if (ui_hit(row(2), x, y)) return SettingsAction::OTA_CHECK;
  return SettingsAction::NONE;
}

void settings_tick(LGFX &lcd, const Persist &p, uint32_t now_ms) {
  if (now_ms - s_last_tick < 1000) return;
  s_last_tick = now_ms;
  lcd.startWrite();
  draw_wifi(lcd, p);
  draw_firmware(lcd, now_ms);
  draw_info(lcd);
  lcd.endWrite();
}

bool settings_timed_out(uint32_t now_ms) { return now_ms - s_last_input >= kTimeoutMs; }
