// CYD-Buddy: setup()/loop() geben nur an app weiter. Beim Start gedrückt halten = Testbild (ui/testscreen.cpp).
#include <Arduino.h>
#include "app.h"
#include "ui/testscreen.h"

void setup() {
  Serial.begin(115200);
  LGFX &lcd = app_lcd();
  lcd.init();
  lcd.setRotation(1);  // Querformat 320x240
  int32_t x, y;
  if (lcd.getTouch(&x, &y)) testscreen_run(lcd);   // kehrt nicht zurück
  app_setup();
}

void loop() { app_loop(); }
