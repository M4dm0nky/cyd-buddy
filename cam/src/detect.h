#pragma once
#include <stdint.h>
#include <esp_camera.h>
#include "logic/cam_xiao.h"
bool detect_init();
bool detect_face(camera_fb_t *fb, CamFace *out, uint32_t *infer_ms);   // true = Gesicht, out = größtes
