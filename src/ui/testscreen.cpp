// Bring-up-Testbild für CYD2USB (aus dem ersten main.cpp, Task 0)
#include "testscreen.h"
#include <Arduino.h>
#include "../config.h"

static void ledSet(bool r, bool g, bool b) {  // aktiv low
  digitalWrite(LED_R, r ? LOW : HIGH);
  digitalWrite(LED_G, g ? LOW : HIGH);
  digitalWrite(LED_B, b ? LOW : HIGH);
}

static void drawTestImage(LGFX &tft) {
  struct Bar { uint32_t col; const char *name; uint32_t txt; };
  const Bar bars[] = {
      {TFT_RED, "ROT", TFT_WHITE},     {TFT_GREEN, "GRUEN", TFT_BLACK},
      {TFT_BLUE, "BLAU", TFT_WHITE},   {TFT_WHITE, "WEISS", TFT_BLACK},
      {TFT_BLACK, "SCHWARZ", TFT_WHITE}};
  tft.fillScreen(TFT_BLACK);
  const int bw = 320 / 5;
  for (int i = 0; i < 5; i++) {
    tft.fillRect(i * bw, 0, bw, 120, bars[i].col);
    tft.setTextColor(bars[i].txt, bars[i].col);
    tft.setTextDatum(middle_center);
    tft.setTextSize(1);
    tft.drawString(bars[i].name, i * bw + bw / 2, 60);
  }
  tft.drawRect(0, 0, 320, 120, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(top_left);
  tft.setTextSize(2);
  tft.drawString("CYD-Buddy Testbild", 10, 135);
  tft.setTextSize(1);
  tft.drawString(String("Panel: ") + LGFX::PANEL_NAME + "  invert=" + String(CYD_INVERT), 10, 165);
  tft.drawString("Touch: Kreis an der Beruehrung", 10, 180);
}

void testscreen_run(LGFX &tft) {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  ledSet(false, false, false);
  pinMode(LDR_PIN, INPUT);

  tft.setBrightness(200);
  drawTestImage(tft);

  ledSet(true, false, false);  delay(250);
  ledSet(false, true, false);  delay(250);
  ledSet(false, false, true);  delay(250);
  ledSet(false, false, false);
  Serial.printf("CYD-Buddy Testbild, Panel %s, invert=%d\n", LGFX::PANEL_NAME, CYD_INVERT);

  uint32_t lastLog = 0;
  for (;;) {
    lgfx::touch_point_t tp;
    if (tft.getTouch(&tp)) {
      tft.fillCircle(tp.x, tp.y, 6, TFT_YELLOW);
    }
    uint32_t now = millis();
    if (now - lastLog >= 500) {
      lastLog = now;
      lgfx::touch_point_t raw;
      int rx = -1, ry = -1;
      if (tft.getTouchRaw(&raw.x, &raw.y)) { rx = raw.x; ry = raw.y; }
      Serial.printf("LDR=%d touch_raw=(%d,%d) heap=%u\n", analogRead(LDR_PIN), rx, ry, (unsigned)ESP.getFreeHeap());
    }
    delay(5);
  }
}
