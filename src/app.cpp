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
#include "ui/menu.h"
#include "ui/clock_view.h"
#include "ui/settings.h"
#include "ui/game_catch.h"
#include "ui/game_simon.h"
#include "ui/ota_card.h"
#include "ota/ota_pull.h"
#include "logic/ota_policy.h"

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

enum class Screen { EYES, MENU, CLOCK, SETTINGS, GAME_CATCH, GAME_SIMON, OTA };
static Screen s_screen = Screen::EYES;

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
  si.update_ready = ota_pull::info().state == ota_pull::AVAILABLE;
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
  if (pressed) mood_touch(s_mood, now);   // jede Berührung hält ihn nachts wach bzw. weckt ihn
  static bool s_was_pressed = false;
  if (pressed && !s_was_pressed) {   // Beginn einer Berührung seriell ausgeben (Touch-Kalibrierung prüfen)
    lgfx::touch_point_t raw;
    s_lcd.getTouchRaw(&raw, 1);
    Serial.printf("touch x=%d y=%d raw=%d,%d\n", (int)x, (int)y, (int)raw.x, (int)raw.y);
  }
  s_was_pressed = pressed;
  // Berührung, die der Chip meldet, die Bibliothek aber verwirft (außerhalb des Bereichs): einmal je Berührung melden
  static bool s_raw_only = false;
  if (!pressed) {
    lgfx::touch_point_t raw;
    const bool raw_down = s_lcd.getTouchRaw(&raw, 1) > 0;
    if (raw_down && !s_raw_only) Serial.printf("touch verworfen raw=%d,%d\n", (int)raw.x, (int)raw.y);
    s_raw_only = raw_down;
  }
  return gesture_feed(s_gesture, pressed, tx, ty, now);
}

static void touch_to_mood(Gesture g, uint32_t now) {
  switch (g) {
    case Gesture::TAP: mood_event(s_mood, MoodEvent::TAP, now); break;
    case Gesture::STROKE: mood_event(s_mood, MoodEvent::STROKE, now); break;
    case Gesture::POKE: mood_event(s_mood, MoodEvent::POKE, now); break;
    default: break;
  }
}

// Stimmung läuft auf jedem Bildschirm weiter (Zeit vergeht), Licht und LED auch.
static void mood_frame(uint32_t now, uint32_t dt) {
  const int hour = current_hour();
  mood_set_dark(s_mood, light_update(now), hour, now);
  mood_night_check(s_mood, hour, now);
  mood_tick(s_mood, dt, now);
  s_emotion = mood_emotion(s_mood, hour, now);
}

static void eyes_frame(uint32_t now, uint32_t dt) {
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

static void go_eyes() {
  s_screen = Screen::EYES;
  status_invalidate();   // Statusstreifen sicher neu zeichnen
}

// Zeile „Firmware“ in den Einstellungen.
static int firmware_line(char *out, size_t n, uint32_t now) {
  const ota_pull::Info i = ota_pull::info();
  return ota_status_line((int)i.state, i.pct, ota_pull::check_busy(), i.last_check_ms, i.last_failed, now, out, n);
}

static void open_settings(uint32_t now) {
  s_screen = Screen::SETTINGS;
  settings_open(s_lcd, s_persist, now);
}

static bool s_manual_ota = false;   // in den Einstellungen "Firmware" getippt: nach der Suche Karte sofort zeigen

static void open_ota_card(uint32_t now) {
  s_screen = Screen::OTA;
  set_backlight(kBacklight[s_persist.brightness]);
  ota_card_open(now);
  ota_card_draw(s_lcd, now);
}

static void screen_frame(uint32_t now, uint32_t dt, Gesture g, int16_t tx, int16_t ty) {
  const bool tap = g == Gesture::TAP;
  switch (s_screen) {
    case Screen::EYES:
      if (g == Gesture::LONG_PRESS && s_emotion != Emotion::SLEEPING) {
        s_screen = Screen::MENU;
        set_backlight(kBacklight[s_persist.brightness]);
        menu_open(s_lcd, now);
        return;
      }
      if (ota_card_wanted(now, s_emotion != Emotion::SLEEPING)) {   // nur bei wachem Display
        open_ota_card(now);
        return;
      }
      touch_to_mood(g, now);
      eyes_frame(now, dt);
      ota_pull::ui_alive();
      return;

    case Screen::MENU: {
      if (menu_timed_out(now)) { go_eyes(); return; }
      if (!tap) return;
      switch (menu_touch(tx, ty, now)) {
        case MenuChoice::CLOCK: s_screen = Screen::CLOCK; clock_open(s_lcd, now); break;
        case MenuChoice::SETTINGS: open_settings(now); break;
        case MenuChoice::CLOSE: go_eyes(); break;
        case MenuChoice::PLAY_CATCH: s_screen = Screen::GAME_CATCH; game_catch_open(s_lcd, now); break;
        case MenuChoice::PLAY_SIMON: s_screen = Screen::GAME_SIMON; game_simon_open(s_lcd, now); break;
        default: break;
      }
      return;
    }

    case Screen::CLOCK:
      if (tap || !clock_update(s_lcd, now)) go_eyes();
      return;

    case Screen::SETTINGS:
      if (settings_timed_out(now)) { go_eyes(); return; }
      if (tap) {
        switch (settings_touch(tx, ty, now)) {
          case SettingsAction::CLOSE: go_eyes(); return;
          case SettingsAction::BRIGHTNESS:
            s_persist.brightness = (uint8_t)((s_persist.brightness + 1) % 3);
            set_backlight(kBacklight[s_persist.brightness]);
            save_now();
            settings_redraw(s_lcd, s_persist);
            break;
          case SettingsAction::PORTAL: net_open_portal(); settings_redraw(s_lcd, s_persist); break;
          case SettingsAction::OTA_CHECK:
            // Manuelles Update: suchen (hebt auch "Später" auf); liegt eins bereit, kommt die Karte sofort (unten).
            ota_pull::check_now();
            s_manual_ota = true;
            settings_redraw(s_lcd, s_persist);
            break;
          default: break;
        }
      }
      if (s_manual_ota && !ota_pull::check_busy()) {
        s_manual_ota = false;
        if (ota_pull::info().state == ota_pull::AVAILABLE) { open_ota_card(now); return; }
      }
      settings_tick(s_lcd, s_persist, now);
      return;

    case Screen::GAME_CATCH:
    case Screen::GAME_SIMON: {
      const GameResult r = s_screen == Screen::GAME_CATCH ? game_catch_update(s_lcd, tap, tx, ty, now)
                                                          : game_simon_update(s_lcd, tap, tx, ty, now);
      if (r == GameResult::RUNNING) return;
      mood_event(s_mood, r == GameResult::WON ? MoodEvent::GAME_WON : MoodEvent::GAME_LOST, now);
      go_eyes();
      return;
    }

    case Screen::OTA: {
      // Beim Laden das Augen-Sprite freigeben (Puffer, TLS); nach Fehler/Schlummern wieder anlegen.
      if (ota_pull::info().state == ota_pull::DOWNLOADING && eyes_ready()) eyes_free();
      if (tap) ota_card_touch(tx, ty, now);
      if (!ota_card_wanted(now, true)) {
        if (!eyes_ready()) eyes_begin(s_lcd);
        s_lcd.fillScreen(TFT_BLACK);
        go_eyes();
        return;
      }
      ota_card_draw(s_lcd, now);
      return;
    }

    default:
      go_eyes();
      return;
  }
}

void app_setup() {
  led_begin();
  light_begin();
  if (!store_load(s_persist)) Serial.println("store: nichts gespeichert, Startwerte");
  mood_init(s_mood, s_persist.needs);
  ota_pull::begin();
  net_task_hook(ota_pull::loop);   // OTA-Abruf läuft im net_task
  settings_firmware_source(firmware_line);
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
  mood_frame(now, dt);
  screen_frame(now, dt, g, tx, ty);

  if (now - s_last_save >= kSaveEveryMs) {
    s_last_save = now;
    save_now();
  }
#if CYD_LOG_LDR   // Einmessen des Lichtsensors: Rohwert jede Sekunde (Build-Flag, danach wieder 0)
  static uint32_t s_last_ldr = 0;
  if (now - s_last_ldr >= 1000) { s_last_ldr = now; Serial.printf("ldr=%d\n", light_raw()); }
#endif
  if (now - s_last_log >= kLogEveryMs) {
    const uint32_t fps = s_frames * 1000UL / (now - s_last_log);
    s_last_log = now;
    s_frames = 0;
    Serial.printf("heap=%u min=%u fps=%u emo=%d\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                  (unsigned)fps, (int)s_emotion);
  }
}
