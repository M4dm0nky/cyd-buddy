#include "clock_view.h"
#include <math.h>
#include <stdio.h>
#include "icons.h"
#include "eyes.h"
#include "ui_common.h"
#include "../net/net.h"
#include "../logic/weather_mood.h"

static constexpr uint32_t kShowMs = 15000;
static constexpr uint32_t kPeekMs = 900;     // so lange schieben sich die Augen hoch
static constexpr int kPeekTop = 196;         // Bereich der Augen unten
static constexpr int kPeekY0 = 262, kPeekY1 = 222;

static uint32_t s_open_ms = 0;
static int s_last_min = -2;
static int s_last_peek_y = -1;

static void draw_time_block(LGFX &d) {
  struct tm t;
  const bool ok = net_local_time(&t);
  d.fillRect(0, 0, 320, kPeekTop, TFT_BLACK);
  char buf[24];
  if (ok) snprintf(buf, sizeof buf, "%02d:%02d", t.tm_hour, t.tm_min);
  else snprintf(buf, sizeof buf, "--:--");
  d.setTextSize(1.4f);
  ui_text(d, buf, 160, 62, &fonts::Font7, d.color888(120, 230, 255), middle_center);
  d.setTextSize(1);
  if (ok) {
    static const char *kDays[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
    snprintf(buf, sizeof buf, "%s %d.%d.", kDays[t.tm_wday % 7], t.tm_mday, t.tm_mon + 1);
    ui_text(d, buf, 160, 128, &fonts::FreeSansBold12pt7b, TFT_WHITE, middle_center);
  } else {
    icon_wifi(d, 160, 128, 34, d.color888(255, 60, 60));
  }
  const NetWeather w = net_weather();
  if (w.valid && weather_fresh(millis(), w.fetched_ms)) {
    const WeatherMood wm = weather_mood(w.code, w.temp_c);
    icon_sky(d, 130, 170, 40, wm.sky, w.is_day);
    snprintf(buf, sizeof buf, "%ld", lroundf(w.temp_c));
    ui_text(d, buf, 158, 170, &fonts::FreeSansBold12pt7b, TFT_WHITE, middle_left);
    d.setFont(&fonts::FreeSansBold12pt7b);
    d.drawCircle(158 + d.textWidth(buf) + 5, 162, 3, TFT_WHITE);
  }
}

static void draw_peek(LGFX &d, int y) {
  d.fillRect(0, kPeekTop, 320, 240 - kPeekTop, TFT_BLACK);
  EyeShape e = eye_expression(Emotion::HAPPY).shape;
  e.gaze_y = -8;   // schaut nach oben zur Uhr
  eyes_draw_pair(d, 160, y, e, Rgb{0, 230, 255}, 0.42f);
}

void clock_open(LGFX &lcd, uint32_t now_ms) {
  s_open_ms = now_ms;
  s_last_min = -2;
  s_last_peek_y = -1;
  lcd.fillScreen(TFT_BLACK);
  clock_update(lcd, now_ms);
}

bool clock_update(LGFX &lcd, uint32_t now_ms) {
  const uint32_t t = now_ms - s_open_ms;
  if (t >= kShowMs) return false;
  struct tm tmv;
  const int min = net_local_time(&tmv) ? tmv.tm_hour * 60 + tmv.tm_min : -1;
  lcd.startWrite();
  if (min != s_last_min) {
    s_last_min = min;
    draw_time_block(lcd);
  }
  const float k = t >= kPeekMs ? 1.0f : (float)t / kPeekMs;
  const int y = kPeekY0 + (int)((kPeekY1 - kPeekY0) * (1 - (1 - k) * (1 - k)));   // bremst am Ende ab
  if (y != s_last_peek_y) {
    s_last_peek_y = y;
    draw_peek(lcd, y);
  }
  lcd.endWrite();
  return true;
}
