#include "game_simon.h"
#include <Arduino.h>
#include "../logic/simon.h"

static constexpr uint32_t kOnMs = 500, kGapMs = 200, kFlashMs = 150, kStartPauseMs = 700;
static const uint8_t kDim[4][3] = {{110, 0, 0}, {0, 100, 30}, {0, 30, 120}, {120, 100, 0}};
static const uint8_t kLit[4][3] = {{255, 70, 70}, {70, 255, 120}, {80, 140, 255}, {255, 230, 60}};

static Simon s_s;
static uint32_t s_show_start = 0;      // Beginn der Anzeige der Folge
static int s_lit = -1;                 // gerade hell gezeichnetes Feld
static int s_flash = -1;               // vom Spieler getipptes Feld, leuchtet kurz
static uint32_t s_flash_until = 0;
static constexpr uint32_t kInputTimeoutMs = 30000;
static uint32_t s_wait_start = 0;      // seit wann auf Eingabe gewartet wird (Beginn von AWAIT_INPUT bzw. letzter Tipp)
static bool s_result_shown = false;
static uint32_t s_result_ms = 0;
static uint8_t s_drawn_len = 0;

static void field_rect(int i, int &x, int &y, int &w, int &h) {
  w = 160; h = 100;
  x = (i & 1) * 160;
  y = GAME_TOP + (i >> 1) * 100;
}

static void draw_field(LGFX &d, int i, bool lit) {
  int x, y, w, h;
  field_rect(i, x, y, w, h);
  const uint8_t *c = lit ? kLit[i] : kDim[i];
  d.fillRoundRect(x + 4, y + 4, w - 8, h - 8, 16, d.color888(c[0], c[1], c[2]));
}

static void draw_progress(LGFX &d) {
  if (s_s.len == s_drawn_len) return;
  s_drawn_len = s_s.len;
  d.fillRect(0, 0, 320, GAME_TOP, TFT_BLACK);
  for (int i = 0; i < SIMON_WIN_LEN; i++)
    d.fillCircle(160 - (SIMON_WIN_LEN - 1) * 11 + i * 22, 20, 7, i < s_s.len ? d.color888(255, 220, 0) : d.color888(60, 60, 80));
}

static int field_at(int16_t x, int16_t y) {
  if (y < GAME_TOP) return -1;
  return (x >= 160 ? 1 : 0) + ((y - GAME_TOP) >= 100 ? 2 : 0);
}

void game_simon_open(LGFX &lcd, uint32_t now_ms) {
  simon_start(s_s, esp_random());
  s_show_start = now_ms + kStartPauseMs;
  s_lit = -1;
  s_flash = -1;
  s_result_shown = false;
  s_drawn_len = 0;
  lcd.startWrite();
  lcd.fillScreen(TFT_BLACK);
  for (int i = 0; i < 4; i++) draw_field(lcd, i, false);
  draw_progress(lcd);
  lcd.endWrite();
}

GameResult game_simon_update(LGFX &lcd, bool tap, int16_t x, int16_t y, uint32_t now_ms) {
  if (s_result_shown) {
    if (now_ms - s_result_ms < GAME_RESULT_MS) return GameResult::RUNNING;
    return s_s.state == SimonState::WON ? GameResult::WON : GameResult::LOST;
  }
  lcd.startWrite();
  if (s_flash >= 0 && (int32_t)(now_ms - s_flash_until) >= 0) {   // Aufleuchten nach 150 ms beenden
    draw_field(lcd, s_flash, false);
    s_flash = -1;
  }
  if (s_s.state == SimonState::SHOW) {
    int want = -1;
    if ((int32_t)(now_ms - s_show_start) >= 0) {
      const uint32_t t = now_ms - s_show_start;
      const uint32_t step = t / (kOnMs + kGapMs);
      if (step >= s_s.len) { simon_shown(s_s); s_wait_start = now_ms; }
      else if (t % (kOnMs + kGapMs) < kOnMs) want = s_s.seq[step];
    }
    if (want != s_lit) {
      if (s_lit >= 0) draw_field(lcd, s_lit, false);
      if (want >= 0) draw_field(lcd, want, true);
      s_lit = want;
    }
  } else if (s_s.state == SimonState::AWAIT_INPUT) {
    const int f = tap ? field_at(x, y) : -1;
    if (f < 0 && now_ms - s_wait_start >= kInputTimeoutMs) {   // 30 s ohne Eingabe: Spiel endet als verloren
      s_s.state = SimonState::LOST;
    } else if (f >= 0) {
      s_wait_start = now_ms;
      if (s_flash >= 0) draw_field(lcd, s_flash, false);
      draw_field(lcd, f, true);
      s_flash = f;
      s_flash_until = now_ms + kFlashMs;
      const uint8_t len_before = s_s.len;
      simon_press(s_s, (uint8_t)f);
      if (s_s.state == SimonState::SHOW && s_s.len != len_before) s_show_start = now_ms + kStartPauseMs;
    }
  }
  draw_progress(lcd);
  lcd.endWrite();

  if (s_s.state == SimonState::WON || s_s.state == SimonState::LOST) {
    s_result_shown = true;
    s_result_ms = now_ms;
    game_show_result(lcd, s_s.state == SimonState::WON);
  }
  return GameResult::RUNNING;
}
