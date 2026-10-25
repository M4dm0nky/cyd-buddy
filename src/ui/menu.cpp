#include "menu.h"
#include "icons.h"
#include "ui_common.h"

static constexpr uint32_t kTimeoutMs = 10000;
// 2x3 Kacheln je 96x90, Abstand 8 (Spec Interaktion): oben Spielen, Füttern, Timer; unten Uhr, Einstellungen, Schließen.
static constexpr int kW = 96, kH = 90, kGap = 8, kX0 = (320 - 3 * kW - 2 * kGap) / 2, kY0 = (240 - 2 * kH - kGap) / 2;
static constexpr UiRect tile(int i) {
  return UiRect{(int16_t)(kX0 + (i % 3) * (kW + kGap)), (int16_t)(kY0 + (i / 3) * (kH + kGap)), kW, kH};
}

static LGFX *s_lcd = nullptr;
static uint32_t s_last_input = 0;
static bool s_games = false;   // zweite Ebene „Spielen“

static void draw() {
  LGFX &d = *s_lcd;
  d.startWrite();
  d.fillScreen(TFT_BLACK);
  const int s = 48;
  auto cx = [](int i) { return tile(i).x + kW / 2; };
  auto cy = [](int i) { return tile(i).y + kH / 2; };
  if (!s_games) {
    ui_tile(d, tile(0), d.color888(30, 160, 70));
    icon_play(d, cx(0), cy(0), s, TFT_WHITE);
    ui_tile(d, tile(1), d.color888(240, 130, 20));
    icon_apple(d, cx(1), cy(1), s + 6);
    ui_tile(d, tile(2), d.color888(170, 40, 120));
    icon_hourglass(d, cx(2), cy(2), s, TFT_WHITE);
    ui_tile(d, tile(3), d.color888(220, 170, 0));
    icon_clock(d, cx(3), cy(3), s, TFT_WHITE);
    ui_tile(d, tile(4), d.color888(130, 70, 200));
    icon_gear(d, cx(4), cy(4), s, TFT_WHITE);
    ui_tile(d, tile(5), d.color888(200, 40, 50));
    icon_close(d, cx(5), cy(5), s, TFT_WHITE);
  } else {
    ui_tile(d, tile(0), d.color888(0, 120, 150));
    icon_target(d, cx(0), cy(0), s + 8, TFT_WHITE);
    ui_tile(d, tile(1), d.color888(40, 40, 60));
    icon_simon(d, cx(1), cy(1), s + 12);
    ui_tile(d, tile(2), d.color888(70, 70, 90));
    icon_back(d, cx(2), cy(2), s, TFT_WHITE);
  }
  d.endWrite();
}

void menu_open(LGFX &lcd, uint32_t now_ms) {
  s_lcd = &lcd;
  s_games = false;
  s_last_input = now_ms;
  draw();
}

MenuChoice menu_touch(int16_t x, int16_t y, uint32_t now_ms) {
  s_last_input = now_ms;
  if (!s_games) {
    if (ui_hit(tile(0), x, y)) { s_games = true; draw(); return MenuChoice::NONE; }
    if (ui_hit(tile(1), x, y)) return MenuChoice::FEED;
    if (ui_hit(tile(2), x, y)) return MenuChoice::TIMER;
    if (ui_hit(tile(3), x, y)) return MenuChoice::CLOCK;
    if (ui_hit(tile(4), x, y)) return MenuChoice::SETTINGS;
    if (ui_hit(tile(5), x, y)) return MenuChoice::CLOSE;
    return MenuChoice::NONE;
  }
  if (ui_hit(tile(0), x, y)) return MenuChoice::PLAY_CATCH;
  if (ui_hit(tile(1), x, y)) return MenuChoice::PLAY_SIMON;
  if (ui_hit(tile(2), x, y)) { s_games = false; draw(); }
  return MenuChoice::NONE;
}

bool menu_timed_out(uint32_t now_ms) { return now_ms - s_last_input >= kTimeoutMs; }
