#include <Arduino.h>
#include <esp_camera.h>
#include "camera.h"
#include "detect.h"

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.printf("# psram=%u\n", (unsigned)ESP.getPsramSize());
  Serial.printf("# camera=%d detect=%d\n", camera_start(), detect_init());
}

void loop() {
  static uint32_t t0 = millis(), frames = 0, ms_sum = 0;
  static CamFace last = {0, 0, 0};
  static bool has = false;
  if (!camera_running()) { delay(500); return; }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;
  uint32_t ms = 0;
  CamFace f;
  has = detect_face(fb, &f, &ms);
  if (has) last = f;
  esp_camera_fb_return(fb);
  frames++; ms_sum += ms;
  if (millis() - t0 >= 1000) {
    Serial.printf("# fps=%u ms=%u face=%d %d,%d,%d\n", (unsigned)frames, (unsigned)(frames ? ms_sum / frames : 0), has, last.x, last.y, last.w);
    t0 = millis(); frames = 0; ms_sum = 0;
  }
}
