#include "ota_card.h"
#include <stdio.h>
#include "icons.h"
#include "eyes.h"
#include "ui_common.h"
#include "../ota/ota_pull.h"
#include "../logic/ota_policy.h"

static constexpr uint32_t kIgnoreTapsMs = 1000;   // Tipps in der ersten Sekunde zählen nicht
static constexpr UiRect kYes = {30, 150, 120, 60};
static constexpr UiRect kLater = {170, 150, 120, 60};

static uint32_t s_shown_ms = 0;
static int s_drawn_state = -1;
static int s_drawn_pct = -1;
static bool s_consented = false;

bool ota_card_wanted(uint32_t now_ms, bool awake) {
  const ota_pull::Info i = ota_pull::info();
  switch (i.state) {
    case ota_pull::AVAILABLE: return ota_notice_due(now_ms, i.snooze_set_ms, awake);
    case ota_pull::DOWNLOADING:
    case ota_pull::VERIFYING:
    case ota_pull::RESTARTING:
    case ota_pull::FAILED: return true;
    default: return false;
  }
}

void ota_card_open(uint32_t now_ms) {
  s_shown_ms = now_ms;
  s_drawn_state = -1;
  s_drawn_pct = -1;
  s_consented = false;
}

static void card_frame(LGFX &d, uint32_t border) {
  d.fillScreen(TFT_BLACK);
  d.fillRoundRect(10, 10, 300, 220, 18, d.color888(20, 26, 40));
  d.drawRoundRect(10, 10, 300, 220, 18, border);
}

static void draw_available(LGFX &d, const ota_pull::Info &i) {
  card_frame(d, d.color888(80, 200, 255));
  icon_update(d, 160, 50, 46, d.color888(80, 200, 255));
  ui_text(d, "Update bereit", 160, 94, &FONT_L, TFT_WHITE, middle_center);
  char buf[48];
  snprintf(buf, sizeof buf, "%s  (%u kB)", i.built, (unsigned)(i.size / 1024));
  ui_text(d, buf, 160, 122, &fonts::Font2, d.color888(190, 200, 220), middle_center);
  ui_tile(d, kYes, d.color888(30, 160, 70));
  ui_text(d, "Ja", kYes.x + kYes.w / 2, kYes.y + kYes.h / 2, &FONT_L, TFT_WHITE, middle_center);
  ui_tile(d, kLater, d.color888(80, 80, 100));
  ui_text(d, "Später", kLater.x + kLater.w / 2, kLater.y + kLater.h / 2, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_waiting(LGFX &d, const char *txt) {
  d.fillScreen(TFT_BLACK);
  EyeShape e = eye_expression(Emotion::CONTENT).shape;
  e.h = 34;              // schmal = konzentriert
  e.radius = 12;
  e.lid_top_cut = 0.25f;
  eyes_draw_pair(d, 160, 80, e, Rgb{0, 220, 255}, 1.0f);
  ui_text(d, txt, 160, 150, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_progress(LGFX &d, int pct) {
  d.drawRoundRect(30, 180, 260, 26, 8, TFT_WHITE);
  d.fillRect(33, 183, 254, 20, TFT_BLACK);
  d.fillRect(33, 183, 254 * pct / 100, 20, d.color888(0, 200, 255));
  char buf[16];
  snprintf(buf, sizeof buf, "%d %%", pct);
  d.fillRect(100, 130, 120, 40, TFT_BLACK);
  ui_text(d, buf, 160, 150, &FONT_L, TFT_WHITE, middle_center);
}

static void draw_failed(LGFX &d, const ota_pull::Info &i) {
  card_frame(d, d.color888(255, 70, 70));
  icon_close(d, 160, 54, 46, d.color888(255, 70, 70));
  ui_text(d, "Update fehlgeschlagen", 160, 104, &FONT_M, TFT_WHITE, middle_center);
  ui_text(d, i.error, 160, 140, &FONT_S, d.color888(255, 180, 180), middle_center);
  ui_text(d, "Antippen schließt", 160, 196, &FONT_S, d.color888(170, 170, 190), middle_center);
}

void ota_card_draw(LGFX &lcd, uint32_t now_ms) {
  (void)now_ms;
  const ota_pull::Info i = ota_pull::info();
  const int st = (int)i.state;
  if (st == s_drawn_state && st != ota_pull::DOWNLOADING) return;
  lcd.startWrite();
  if (st != s_drawn_state) {
    s_drawn_state = st;
    s_drawn_pct = -1;
    switch (i.state) {
      case ota_pull::AVAILABLE:
        if (s_consented) draw_waiting(lcd, "Starte ...");
        else draw_available(lcd, i);
        break;
      case ota_pull::DOWNLOADING: draw_waiting(lcd, ""); break;
      case ota_pull::VERIFYING: draw_waiting(lcd, "Prüfe die Datei ..."); break;
      case ota_pull::RESTARTING: draw_waiting(lcd, "Neustart ..."); break;
      case ota_pull::FAILED: draw_failed(lcd, i); break;
      default: break;
    }
  }
  if (i.state == ota_pull::DOWNLOADING && i.pct != s_drawn_pct) {
    s_drawn_pct = i.pct;
    draw_progress(lcd, i.pct);
  }
  lcd.endWrite();
}

void ota_card_touch(int16_t x, int16_t y, uint32_t now_ms) {
  if (now_ms - s_shown_ms < kIgnoreTapsMs) return;
  const ota_pull::Info i = ota_pull::info();
  if (i.state == ota_pull::AVAILABLE && !s_consented) {
    if (ui_hit(kYes, x, y)) {
      s_consented = true;
      s_drawn_state = -1;   // „Starte …“ zeichnen
      ota_pull::consent();
    } else {
      ota_pull::snooze();
    }
  } else if (i.state == ota_pull::FAILED) {
    ota_pull::dismiss_failure();
  }
}
