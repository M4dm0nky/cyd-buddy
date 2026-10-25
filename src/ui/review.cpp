#include "review.h"
#include <stdio.h>
#include "icons.h"
#include "eyes.h"
#include "ui_common.h"
#include "../logic/face_model.h"

static constexpr uint32_t kShowMs = 12000;
static constexpr uint32_t kIgnoreTapsMs = 1000;   // Tipps in der ersten Sekunde schließen nicht (wie bei der OTA-Karte)
static uint32_t s_open_ms = 0;

static void line(LGFX &d, int i, void (*icon)(LGFX &, int, int), const char *txt) {
  const int y = 58 + i * 28;
  icon(d, 40, y);
  ui_text(d, txt, 66, y, &FONT_M, TFT_WHITE, middle_left);
}

void review_open(LGFX &lcd, const DayStats &st, uint32_t now_ms) {
  s_open_ms = now_ms;
  char b[32];
  lcd.startWrite();
  lcd.fillScreen(TFT_BLACK);
  ui_text(lcd, "Dein Tag mit mir", 160, 20, &FONT_L, lcd.color888(255, 210, 120), middle_center);
  snprintf(b, sizeof b, "%u gestreichelt", (unsigned)st.strokes);
  line(lcd, 0, [](LGFX &d, int x, int y) { icon_heart(d, x, y, 20, d.color888(255, 80, 170)); }, b);
  snprintf(b, sizeof b, "%u gestupst", (unsigned)st.pokes);
  line(lcd, 1, [](LGFX &d, int x, int y) { icon_bolt(d, x, y, 22, d.color888(255, 220, 0)); }, b);
  snprintf(b, sizeof b, "%u / %u Spiele gewonnen", (unsigned)st.wins, (unsigned)st.games);
  line(lcd, 2, [](LGFX &d, int x, int y) { icon_play(d, x, y, 20, d.color888(60, 220, 120)); }, b);
  snprintf(b, sizeof b, "%u gegessen", (unsigned)st.meals);
  line(lcd, 3, [](LGFX &d, int x, int y) { icon_apple(d, x, y, 24); }, b);
  snprintf(b, sizeof b, "%u Pomodoros", (unsigned)st.pomodoros);
  line(lcd, 4, [](LGFX &d, int x, int y) { icon_hourglass(d, x, y, 22, d.color888(255, 90, 140)); }, b);
  // kleines Augenpaar mit dem Gefühl des Tages
  const Expression x = eye_expression(daystats_mood(st));
  eyes_draw_pair(lcd, 160, 208, x.shape, x.color, 0.42f);
  lcd.endWrite();
}

bool review_tap_closes(uint32_t now_ms) { return now_ms - s_open_ms >= kIgnoreTapsMs; }

bool review_update(LGFX &lcd, uint32_t now_ms) {
  (void)lcd;
  return now_ms - s_open_ms < kShowMs;
}
