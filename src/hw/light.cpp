#include "light.h"
#include <Arduino.h>
#include "../config.h"

#ifndef LDR_DARK_IS_HIGH
#define LDR_DARK_IS_HIGH 1   // Vorgabe, falls config.h nichts setzt: 1 = höherer Rohwert bedeutet dunkel (config.h setzt es)
#endif

static constexpr uint8_t kSamples = 16;
static constexpr uint32_t kIntervalMs = 60;

static uint16_t s_buf[kSamples];
static uint8_t s_n = 0, s_pos = 0;
static uint32_t s_sum = 0;
static uint32_t s_last_ms = 0;
static bool s_measured = false;
static int s_avg = 0;
static bool s_dark = false;

void light_begin() {
  pinMode(LDR_PIN, INPUT);
  s_n = s_pos = 0;
  s_sum = 0;
  s_measured = false;
  s_dark = false;
}

bool light_update(uint32_t now_ms) {
  if (s_measured && now_ms - s_last_ms < kIntervalMs) return s_dark;
  s_measured = true;
  s_last_ms = now_ms;
  const uint16_t v = (uint16_t)analogRead(LDR_PIN);
  if (s_n == kSamples) s_sum -= s_buf[s_pos];
  else s_n++;
  s_buf[s_pos] = v;
  s_sum += v;
  s_pos = (s_pos + 1) % kSamples;
  s_avg = (int)(s_sum / s_n);

  // Schwelle <= 0 = Sensor abgeschaltet: dann nie "dunkel", sonst schliefe der Buddy immer.
  if (LDR_DARK_THRESHOLD <= 0 || s_n < kSamples) return s_dark;
#if LDR_DARK_IS_HIGH
  if (!s_dark && s_avg > LDR_DARK_THRESHOLD + LDR_HYSTERESIS) s_dark = true;
  else if (s_dark && s_avg < LDR_DARK_THRESHOLD - LDR_HYSTERESIS) s_dark = false;
#else
  if (!s_dark && s_avg < LDR_DARK_THRESHOLD - LDR_HYSTERESIS) s_dark = true;
  else if (s_dark && s_avg > LDR_DARK_THRESHOLD + LDR_HYSTERESIS) s_dark = false;
#endif
  return s_dark;
}

int light_raw() { return s_avg; }
