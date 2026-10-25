#include "menu.h"
#include "icons.h"
#include "ui_common.h"

static constexpr uint32_t kTimeoutMs = 10000;
static constexpr UiRect kTiles[4] = {{10, 15, 140, 100}, {170, 15, 140, 100}, {10, 125, 140, 100}, {170, 125, 140, 100}};
static constexpr UiRect kBack = {10, 125, 300, 100};

static LGFX *s_lcd = nullptr;
static uint32_t s_last_input = 0;
static bool s_games = false;   // zweite Ebene „Spielen“

static void draw() {
  LGFX &d = *s_lcd;
  d.startWrite();
  d.fillScreen(TFT_BLACK);
  const int s = 56;
  if (!s_games) {
    ui_tile(d, kTiles[0], d.color888(30, 160, 70));
    icon_play(d, kTiles[0].x + kTiles[0].w / 2, kTiles[0].y + kTiles[0].h / 2, s, TFT_WHITE);
    ui_tile(d, kTiles[1], d.color888(220, 170, 0));
    icon_clock(d, kTiles[1].x + kTiles[1].w / 2, kTiles[1].y + kTiles[1].h / 2, s, TFT_WHITE);
    ui_tile(d, kTiles[2], d.color888(130, 70, 200));
    icon_gear(d, kTiles[2].x + kTiles[2].w / 2, kTiles[2].y + kTiles[2].h / 2, s, TFT_WHITE);
    ui_tile(d, kTiles[3], d.color888(200, 40, 50));
    icon_close(d, kTiles[3].x + kTiles[3].w / 2, kTiles[3].y + kTiles[3].h / 2, s, TFT_WHITE);
  } else {
    ui_tile(d, kTiles[0], d.color888(0, 120, 150));
    icon_target(d, kTiles[0].x + kTiles[0].w / 2, kTiles[0].y + kTiles[0].h / 2, s + 10, TFT_WHITE);
    ui_tile(d, kTiles[1], d.color888(40, 40, 60));
    icon_simon(d, kTiles[1].x + kTiles[1].w / 2, kTiles[1].y + kTiles[1].h / 2, s + 16);
    ui_tile(d, kBack, d.color888(70, 70, 90));
    icon_back(d, kBack.x + kBack.w / 2, kBack.y + kBack.h / 2, s, TFT_WHITE);
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
    if (ui_hit(kTiles[0], x, y)) { s_games = true; draw(); return MenuChoice::NONE; }
    if (ui_hit(kTiles[1], x, y)) return MenuChoice::CLOCK;
    if (ui_hit(kTiles[2], x, y)) return MenuChoice::SETTINGS;
    if (ui_hit(kTiles[3], x, y)) return MenuChoice::CLOSE;
    return MenuChoice::NONE;
  }
  if (ui_hit(kTiles[0], x, y)) return MenuChoice::PLAY_CATCH;
  if (ui_hit(kTiles[1], x, y)) return MenuChoice::PLAY_SIMON;
  if (ui_hit(kBack, x, y)) { s_games = false; draw(); }
  return MenuChoice::NONE;
}

bool menu_timed_out(uint32_t now_ms) { return now_ms - s_last_input >= kTimeoutMs; }
