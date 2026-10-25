#include "app.h"
#include <Arduino.h>
#include "config.h"
#include "logic/mood.h"
#include "logic/gesture.h"
#include "logic/eye_model.h"
#include "logic/weather_mood.h"
#include "hw/led.h"
#include "hw/light.h"
#include "hw/store.h"
#include "ui/eyes.h"
#include "ui/status_bar.h"
#include "net/net.h"

static LGFX s_lcd;
LGFX &app_lcd() { return s_lcd; }

static constexpr uint32_t kFrameMs = 33;              // ~30 Bilder/s
static constexpr uint32_t kSaveEveryMs = 10UL * 60000UL;
static constexpr uint32_t kLogEveryMs = 60000UL;
static constexpr uint32_t kStormEveryMs = 4UL * 60000UL;
static constexpr uint32_t kStormForMs = 1000;
static constexpr uint8_t kLedPct = 15;
static constexpr uint8_t kSleepBacklight = 26;        // 10 %
static const uint8_t kBacklight[3] = {77, 153, 255};  // 30 / 60 / 100 %

static Persist s_persist;
static MoodState s_mood;
static GestureState s_gesture;
static Blinker s_blink;
static Gazer s_gaze;
static EyeShape s_shape;
static Rgb s_color;
static uint32_t s_last_frame = 0, s_last_save = 0, s_last_log = 0;
static uint32_t s_frames = 0;
static uint8_t s_backlight = 0;
static Emotion s_emotion = Emotion::CONTENT;

static bool s_elapsed_applied = false;

static int current_hour() { return net_hour(); }

static void set_backlight(uint8_t v) {
  if (v == s_backlight) return;
  s_backlight = v;
  s_lcd.setBrightness(v);
}

static void save_now() {
  s_persist.needs = s_mood.n;
  const uint32_t epoch = net_epoch();
  if (epoch) s_persist.epoch = epoch;
  store_save(s_persist);
}

// Frisches Wetter (höchstens 3 h alt) oder false.
static bool fresh_weather(uint32_t now, WeatherMood *wm, NetWeather *w) {
  *w = net_weather();
  if (!w->valid || !weather_fresh(now, w->fetched_ms)) return false;
  *wm = weather_mood(w->code, w->temp_c);
  return true;
}

static StatusInfo status_info(uint32_t now) {
  StatusInfo si{};
  struct tm t;
  si.time_ok = net_local_time(&t);
  si.hour = si.time_ok ? t.tm_hour : 0;
  si.minute = si.time_ok ? t.tm_min : 0;
  si.wifi_ok = net_wifi_ok();
  si.portal = net_portal_active();
  WeatherMood wm;
  NetWeather w;
  si.weather_ok = fresh_weather(now, &wm, &w);
  si.sky = si.weather_ok ? wm.sky : Sky::CLOUDY;
  si.is_day = si.weather_ok ? w.is_day : true;
  si.temp_c = si.weather_ok ? w.temp_c : 0;
  si.update_ready = false;
  return si;
}

// Einmal, sobald die Uhrzeit bekannt ist: Aus-Zeit seit dem letzten Speichern wie Schlaf nachrechnen.
static void apply_elapsed_once() {
  if (s_elapsed_applied || !net_time_ok()) return;
  s_elapsed_applied = true;
  const uint32_t now_epoch = net_epoch();
  if (s_persist.epoch && now_epoch > s_persist.epoch) {
    const uint32_t secs = now_epoch - s_persist.epoch;
    mood_apply_elapsed(s_mood, secs);
    Serial.printf("mood: %u s aus, Energie jetzt %.1f\n", (unsigned)secs, s_mood.n.energy);
  }
  s_persist.epoch = now_epoch;
}

// Touch lesen und Gesten an die Stimmung weitergeben. Liefert die Geste (für Menü usw.).
static Gesture handle_touch(uint32_t now, int16_t &tx, int16_t &ty) {
  int32_t x = 0, y = 0;
  const bool pressed = s_lcd.getTouch(&x, &y) > 0;
  tx = (int16_t)x;
  ty = (int16_t)y;
  return gesture_feed(s_gesture, pressed, tx, ty, now);
}

static void eyes_frame(uint32_t now, uint32_t dt, Gesture g) {
  switch (g) {
    case Gesture::TAP: mood_event(s_mood, MoodEvent::TAP, now); break;
    case Gesture::STROKE: mood_event(s_mood, MoodEvent::STROKE, now); break;
    case Gesture::POKE: mood_event(s_mood, MoodEvent::POKE, now); break;
    case Gesture::LONG_PRESS: break;   // Menü ab Task 10
    default: break;
  }

  const int hour = current_hour();
  mood_set_dark(s_mood, light_update(now), hour, now);
  mood_tick(s_mood, dt, now);
  s_emotion = mood_emotion(s_mood, hour, now);
  const bool sleeping = s_emotion == Emotion::SLEEPING;

  WeatherMood wm;
  NetWeather w;
  const bool have_weather = fresh_weather(now, &wm, &w);
  const uint32_t ov = overlays_for(s_emotion, have_weather ? &wm : nullptr, have_weather ? w.is_day : true);

  Expression target = eye_expression(s_emotion);
  if ((ov & OV_STORM) && now % kStormEveryMs < kStormForMs) target.shape = eye_expression(Emotion::SURPRISED).shape;
  gazer_update(s_gaze, now);
  target.shape.gaze_x = sleeping ? 0 : s_gaze.gx;
  target.shape.gaze_y = sleeping ? 0 : s_gaze.gy;
  s_shape = eye_tween(s_shape, target.shape, dt);
  s_color = rgb_tween(s_color, target.color, dt);

  EyeShape draw = s_shape;
  if (!sleeping) draw.open *= blinker_open(s_blink, now);
  eyes_draw(s_lcd, draw, s_color, ov, now);
  status_draw(s_lcd, status_info(now));

  led_set(sleeping ? Rgb{0, 0, 0} : s_color, kLedPct);
  set_backlight(sleeping ? kSleepBacklight : kBacklight[s_persist.brightness]);
}

void app_setup() {
  led_begin();
  light_begin();
  if (!store_load(s_persist)) Serial.println("store: nichts gespeichert, Startwerte");
  mood_init(s_mood, s_persist.needs);
  net_begin(&s_persist);
  gesture_init(s_gesture);
  const uint32_t now = millis();
  const uint32_t seed = esp_random();
  blinker_init(s_blink, seed, now);
  gazer_init(s_gaze, seed ^ 0x9E3779B9u, now);
  const Expression start = eye_expression(Emotion::CONTENT);
  s_shape = start.shape;
  s_color = start.color;

  s_lcd.fillScreen(TFT_BLACK);
  eyes_begin(s_lcd);
  status_invalidate();
  set_backlight(kBacklight[s_persist.brightness]);
  s_last_frame = s_last_save = s_last_log = now;
  Serial.printf("CYD-Buddy gestartet, Panel %s, heap=%u\n", LGFX::PANEL_NAME, (unsigned)ESP.getFreeHeap());
}

void app_loop() {
  const uint32_t now = millis();
  const uint32_t dt = now - s_last_frame;
  if (dt < kFrameMs) {
    delay(kFrameMs - dt);
    return;
  }
  s_last_frame = now;
  s_frames++;

  net_loop();
  apply_elapsed_once();

  int16_t tx, ty;
  const Gesture g = handle_touch(now, tx, ty);
  eyes_frame(now, dt, g);

  if (now - s_last_save >= kSaveEveryMs) {
    s_last_save = now;
    save_now();
  }
  if (now - s_last_log >= kLogEveryMs) {
    const uint32_t fps = s_frames * 1000UL / (now - s_last_log);
    s_last_log = now;
    s_frames = 0;
    Serial.printf("heap=%u min=%u fps=%u emo=%d\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                  (unsigned)fps, (int)s_emotion);
  }
}
