#include "led.h"
#include <Arduino.h>
#include "../config.h"

static constexpr uint32_t kFreq = 5000;
static constexpr uint8_t kBits = 8;

static void write_channel(int pin, uint8_t v, uint8_t max_pct) {
  const uint32_t scaled = (uint32_t)v * (max_pct > 100 ? 100 : max_pct) / 100;
  ledcWrite(pin, 255 - scaled);   // aktiv low: 255 = aus
}

void led_begin() {
  ledcAttach(LED_R, kFreq, kBits);
  ledcAttach(LED_G, kFreq, kBits);
  ledcAttach(LED_B, kFreq, kBits);
  led_set(Rgb{0, 0, 0}, 0);
}

void led_set(Rgb c, uint8_t max_pct) {
  write_channel(LED_R, c.r, max_pct);
  write_channel(LED_G, c.g, max_pct);
  write_channel(LED_B, c.b, max_pct);
}
