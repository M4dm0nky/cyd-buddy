#pragma once
#include <stdint.h>
#include "../hw/display.h"

// Update-Karte („Update bereit“: Ja / Später) bzw. Fortschritt mit konzentrierten Augen, Fehlerkarte.
bool ota_card_wanted(uint32_t now_ms, bool awake);   // AVAILABLE && ota_notice_due(...) oder DOWNLOADING/VERIFYING/RESTARTING/FAILED
void ota_card_open(uint32_t now_ms);                 // beim Wechsel auf den OTA-Bildschirm (zeichnet beim nächsten draw neu)
void ota_card_draw(LGFX &lcd, uint32_t now_ms);      // Karte bzw. Fortschritt; zeichnet nur bei Änderung
void ota_card_touch(int16_t x, int16_t y, uint32_t now_ms);   // Ja -> consent(), Später/daneben -> snooze(), FAILED -> dismiss_failure()
