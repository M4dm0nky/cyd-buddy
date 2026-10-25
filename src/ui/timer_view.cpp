#include "timer_view.h"
#include <stdio.h>
#include "icons.h"
#include "ui_common.h"

static constexpr uint32_t kTimeoutMs = 10000;
static constexpr UiRect kMin[3] = {{8, 14, 96, 130}, {112, 14, 96, 130}, {216, 14, 96, 130}};
static constexpr UiRect kBack = {8, 152, 304, 80};
static constexpr UiRect kStop = {8, 14, 304, 130};
static constexpr uint8_t kMinutes[3] = {15, 25, 45};
static bool s_running = false;
static uint32_t s_last_input = 0;

void timer_open(LGFX &d, bool running, uint32_t now_ms) {
  s_running = running;
  s_last_input = now_ms;
  const uint32_t col = d.color888(170, 40, 120);
  d.startWrite();
  d.fillScreen(TFT_BLACK);
  if (running) {
    ui_tile(d, kStop, d.color888(200, 40, 50));
    icon_stop(d, kStop.x + kStop.w / 2, kStop.y + kStop.h / 2, 80, TFT_WHITE);
  } else {
    char b[4];
    for (int i = 0; i < 3; i++) {
      ui_tile(d, kMin[i], col);
      snprintf(b, sizeof b, "%u", kMinutes[i]);   // Zahlen sind hier erlaubt (Spec)
      ui_text(d, b, kMin[i].x + kMin[i].w / 2, kMin[i].y + 46, &fonts::FreeSansBold24pt7b, TFT_WHITE, middle_center);
      icon_hourglass(d, kMin[i].x + kMin[i].w / 2, kMin[i].y + 100, 34, TFT_WHITE);
    }
  }
  ui_tile(d, kBack, d.color888(70, 70, 90));
  icon_back(d, kBack.x + kBack.w / 2, kBack.y + kBack.h / 2, 48, TFT_WHITE);
  d.endWrite();
}

int timer_touch(int16_t x, int16_t y, uint32_t now_ms) {
  s_last_input = now_ms;
  if (ui_hit(kBack, x, y)) return TIMER_BACK;
  if (s_running) return ui_hit(kStop, x, y) ? TIMER_STOP : TIMER_NONE;
  for (int i = 0; i < 3; i++) if (ui_hit(kMin[i], x, y)) return kMinutes[i];
  return TIMER_NONE;
}

bool timer_timed_out(uint32_t now_ms) { return now_ms - s_last_input >= kTimeoutMs; }
