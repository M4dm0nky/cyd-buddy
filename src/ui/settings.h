#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../hw/display.h"
#include "../hw/store.h"

// Einstellungen: Helligkeit (3 Stufen), WLAN/Ort (Portal), Firmware, Info; Schließen oben rechts.
enum class SettingsAction : uint8_t { NONE, CLOSE, BRIGHTNESS, PORTAL, OTA_CHECK };
void settings_open(LGFX &lcd, const Persist &p, uint32_t now_ms);
SettingsAction settings_touch(int16_t x, int16_t y, uint32_t now_ms);
void settings_redraw(LGFX &lcd, const Persist &p);   // nach Änderung
void settings_tick(LGFX &lcd, const Persist &p, uint32_t now_ms);   // Firmware-/Info-Zeile jede Sekunde auffrischen
bool settings_timed_out(uint32_t now_ms);            // 30 s ohne Eingabe
// Text der Firmware-Zeile; Rückgabe 0 neutral, 1 gut, 2 Problem (wie ota_status_line). Ohne Quelle: „–“.
void settings_firmware_source(int (*fn)(char *out, size_t n, uint32_t now_ms));
