// CYD-Buddy: setup()/loop() geben nur an app weiter.
// Touch: beim ersten Start (oder Finger beim Einschalten auf dem Display) Kalibrierung mit vier Eckpunkten, Werte im NVS.
#include <Arduino.h>
#include "app.h"
#include "hw/store.h"
#include "ui/fonts.h"

// Vier Pfeile nacheinander in den Ecken antippen (LovyanGFX). Vorher warten, bis kein Finger mehr liegt.
static void touch_calibrate(LGFX &lcd, uint16_t cal[8]) {
  int32_t x, y;
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  lcd.setTextDatum(middle_center);
  lcd.setFont(&FONT_L);
  lcd.drawString("Touch einrichten", 160, 95);
  lcd.setFont(&FONT_M);
  lcd.drawString("Finger loslassen, dann", 160, 130);
  lcd.drawString("die Pfeile in den Ecken antippen", 160, 152);
  while (lcd.getTouch(&x, &y)) delay(20);
  delay(1500);
  lcd.calibrateTouch(cal, TFT_YELLOW, TFT_BLACK, 24);
  lcd.fillScreen(TFT_BLACK);
  Serial.printf("touch: kalibriert %u,%u %u,%u %u,%u %u,%u\n", cal[0], cal[1], cal[2], cal[3], cal[4], cal[5], cal[6], cal[7]);
}

void setup() {
  Serial.begin(115200);
  LGFX &lcd = app_lcd();
  lcd.init();
  lcd.setRotation(1);  // Querformat 320x240
  uint16_t cal[8];
  int32_t x, y;
  const bool force = lcd.getTouch(&x, &y);   // Finger beim Einschalten: neu kalibrieren
  if (!force && store_load_touch(cal)) {
    lcd.setTouchCalibrate(cal);
  } else {
    touch_calibrate(lcd, cal);
    store_save_touch(cal);
  }
  app_setup();
}

void loop() { app_loop(); }
