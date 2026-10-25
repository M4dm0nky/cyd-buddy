#include "game_catch.h"
#include <Arduino.h>
#include "../logic/catch_game.h"

static CatchGame s_g;
static int16_t s_drawn_x = -1, s_drawn_y = -1;
static uint16_t s_drawn_hits = 0xFFFF;
static int s_drawn_bar = -1;
static bool s_result_shown = false;
static uint32_t s_result_ms = 0;

static void draw_eye(LGFX &d, int x, int y, uint32_t col) {
  const int w = CATCH_R * 3 / 2, h = CATCH_R * 2;
  d.fillRoundRect(x - w / 2, y - h / 2, w, h, CATCH_R / 2, col);
}

static void draw_hud(LGFX &d, uint32_t now) {
  const uint32_t el = now - s_g.start_ms;
  const int bar = el >= CATCH_ROUND_MS ? 0 : (int)(300 * (CATCH_ROUND_MS - el) / CATCH_ROUND_MS);
  if (bar != s_drawn_bar) {
    s_drawn_bar = bar;
    d.fillRect(10, 6, 300, 10, d.color888(40, 40, 60));
    d.fillRect(10, 6, bar, 10, bar > 75 ? d.color888(60, 220, 120) : d.color888(255, 90, 60));
  }
  if (s_g.hits != s_drawn_hits) {
    s_drawn_hits = s_g.hits;
    d.fillRect(0, 20, 320, 18, TFT_BLACK);
    const int n = s_g.hits > 16 ? 16 : s_g.hits;
    for (int i = 0; i < CATCH_WIN_HITS || i < n; i++) {
      const uint32_t col = i < n ? d.color888(255, 220, 0) : d.color888(60, 60, 80);
      d.fillCircle(20 + i * 18, 29, 6, col);
    }
  }
}

void game_catch_open(LGFX &lcd, uint32_t now_ms) {
  catch_start(s_g, esp_random(), now_ms);
  s_drawn_x = s_drawn_y = -1;
  s_drawn_hits = 0xFFFF;
  s_drawn_bar = -1;
  s_result_shown = false;
  lcd.fillScreen(TFT_BLACK);
}

GameResult game_catch_update(LGFX &lcd, bool tap, int16_t x, int16_t y, uint32_t now_ms) {
  if (s_result_shown) {
    if (now_ms - s_result_ms < GAME_RESULT_MS) return GameResult::RUNNING;
    return catch_won(s_g) ? GameResult::WON : GameResult::LOST;
  }
  if (tap) catch_tap(s_g, x, (int16_t)(y - GAME_TOP), now_ms);
  catch_tick(s_g, now_ms);
  if (s_g.over) {
    s_result_shown = true;
    s_result_ms = now_ms;
    game_show_result(lcd, catch_won(s_g));
    return GameResult::RUNNING;
  }
  lcd.startWrite();
  if (s_g.x != s_drawn_x || s_g.y != s_drawn_y) {
    if (s_drawn_x >= 0) lcd.fillCircle(s_drawn_x, s_drawn_y + GAME_TOP, CATCH_R + 2, TFT_BLACK);   // alte Stelle übermalen
    draw_eye(lcd, s_g.x, s_g.y + GAME_TOP, lcd.color888(0, 220, 255));
    s_drawn_x = s_g.x;
    s_drawn_y = s_g.y;
  }
  draw_hud(lcd, now_ms);
  lcd.endWrite();
  return GameResult::RUNNING;
}
