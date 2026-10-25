// CYD-Buddy: setup()/loop() geben nur an app weiter.
// Touch: beim ersten Start (oder Finger beim Einschalten auf dem Display) Kalibrierung mit vier Eckpunkten, Werte im NVS.
#include <Arduino.h>
#include "app.h"
#include "hw/store.h"
#include "ota/ota_pull.h"
#include "ui/fonts.h"

// Rollback wirksam machen: Der Core markiert ein frisch per OTA geflashtes Image (Zustand PENDING_VERIFY) sonst schon vor
// setup() als gut. Mit true bleibt es offen, bis ota_pull::health_tick() es nach 20 s laufender Augen-Schleife bestätigt;
// ein Neustart davor lässt den Bootloader auf die vorige Firmware zurückgehen. Core-Symbol ist C (esp32-hal-misc.c, schwach).
// Nach einem Kabel-Flash ist der Zustand UNDEFINED/VALID: dann ändert sich nichts.
extern "C" bool verifyRollbackLater() { return true; }

// Plausibel = nicht alles 0/gleich und beide Achsen deutlich gespreizt (gemessen: x 474..3679, y 310..3703).
static bool cal_plausible(const uint16_t cal[8]) {
  uint16_t xmin = 0xFFFF, xmax = 0, ymin = 0xFFFF, ymax = 0;
  for (int i = 0; i < 8; i += 2) {
    if (cal[i] < xmin) xmin = cal[i];
    if (cal[i] > xmax) xmax = cal[i];
    if (cal[i + 1] < ymin) ymin = cal[i + 1];
    if (cal[i + 1] > ymax) ymax = cal[i + 1];
  }
  return (xmax - xmin) >= 1000 && (ymax - ymin) >= 1000;
}

// Vier Pfeile nacheinander in den Ecken antippen (LovyanGFX). Vorher höchstens 10 s warten, bis kein Finger mehr liegt.
// Ergebnis unplausibel (z. B. Finger blieb liegen) -> bis zu dreimal neu; true = brauchbare Werte.
static bool touch_calibrate(LGFX &lcd, uint16_t cal[8]) {
  for (int attempt = 0; attempt < 3; attempt++) {
    int32_t x, y;
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextDatum(middle_center);
    lcd.setFont(&FONT_L);
    lcd.drawString("Touch einrichten", 160, 95);
    lcd.setFont(&FONT_M);
    lcd.drawString("Finger loslassen, dann", 160, 130);
    lcd.drawString("die Pfeile in den Ecken antippen", 160, 152);
    const uint32_t t0 = millis();
    while (lcd.getTouch(&x, &y) && millis() - t0 < 10000UL) delay(20);
    delay(1500);
    lcd.calibrateTouch(cal, TFT_YELLOW, TFT_BLACK, 24);
    lcd.fillScreen(TFT_BLACK);
    Serial.printf("touch: kalibriert %u,%u %u,%u %u,%u %u,%u\n", cal[0], cal[1], cal[2], cal[3], cal[4], cal[5], cal[6], cal[7]);
    if (cal_plausible(cal)) return true;
    Serial.println("touch: Werte unplausibel, neu");
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  LGFX &lcd = app_lcd();
  lcd.init();
  lcd.setRotation(1);  // Querformat 320x240
  uint16_t cal[8];
  int32_t x, y;
  const bool force = lcd.getTouch(&x, &y);   // Finger beim Einschalten: neu kalibrieren
  if (!force && store_load_touch(cal) && cal_plausible(cal)) {
    lcd.setTouchCalibrate(cal);
  } else {
    if (touch_calibrate(lcd, cal)) store_save_touch(cal);   // unbrauchbar: für diese Sitzung nutzen, nicht speichern -> nächster Start fragt neu
    else lcd.setTouchCalibrate(cal);
  }
  app_setup();
}

void loop() {
  app_loop();
  ota_pull::health_tick();   // Gesundheitsnachweis nach OTA, unabhängig vom WLAN (billig; nach dem ersten Erfolg sofort zurück)
}
