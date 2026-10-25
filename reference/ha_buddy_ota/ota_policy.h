#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// Entscheidungen des Firmware-Updates per Abruf (src/ota/ota_pull.cpp). Reine Logik ohne Arduino-/LVGL-Includes (Host-Test, siehe selftest).
// Das Gerät holt die Datei selbst, aber nur nach Zustimmung am Display; was es bootet, prüft es am Image selbst, nicht an den Angaben im Manifest.

// App-Image einer Arduino-/IDF-Firmware: Kopf 24 Byte (Magic 0xE9, Chip-ID bei Byte 12), dann Segmentkopf (8 Byte), dann esp_app_desc_t ab Byte 32.
// esp_app_desc_t: magic 0xABCD5432 (0), project_name (48..79), app_elf_sha256 (144..175).
constexpr size_t OTA_IMG_CHECK_BYTES = 176;    // so viel vom Anfang der Datei reicht für die Prüfung
constexpr uint16_t OTA_CHIP_ESP32S3 = 9;

enum OtaImg : uint8_t { OTA_IMG_OK = 0, OTA_IMG_SHORT, OTA_IMG_MAGIC, OTA_IMG_CHIP, OTA_IMG_DESC, OTA_IMG_PROJECT };

static inline bool ota_hex_ok(const char *s, size_t n) {
  if (!s || strlen(s) != n) return false;
  for (size_t i = 0; i < n; i++) if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f'))) return false;
  return true;
}

// Prüft die ersten Bytes eines Images. elf_hex (65 Byte) bekommt bei OTA_IMG_OK die ELF-SHA-256 des Images (64 Hexziffern).
static inline OtaImg ota_image_check(const uint8_t *b, size_t n, const char *project, char *elf_hex) {
  if (!b || n < OTA_IMG_CHECK_BYTES) return OTA_IMG_SHORT;
  if (b[0] != 0xE9) return OTA_IMG_MAGIC;
  if ((uint16_t)(b[12] | (b[13] << 8)) != OTA_CHIP_ESP32S3) return OTA_IMG_CHIP;
  const uint32_t magic = (uint32_t)b[32] | ((uint32_t)b[33] << 8) | ((uint32_t)b[34] << 16) | ((uint32_t)b[35] << 24);
  if (magic != 0xABCD5432u) return OTA_IMG_DESC;
  char name[33];
  memcpy(name, b + 32 + 48, 32);
  name[32] = '\0';
  if (!project || strcmp(name, project) != 0) return OTA_IMG_PROJECT;
  static const char kHex[] = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    elf_hex[2 * i] = kHex[b[32 + 144 + i] >> 4];
    elf_hex[2 * i + 1] = kHex[b[32 + 144 + i] & 15];
  }
  elf_hex[64] = '\0';
  return OTA_IMG_OK;
}

static inline const char *ota_image_error(OtaImg r) {
  switch (r) {
    case OTA_IMG_OK: return "ok";
    case OTA_IMG_SHORT: return "Datei zu kurz";
    case OTA_IMG_MAGIC: return "kein Firmware-Image";
    case OTA_IMG_CHIP: return "falscher Chip";
    case OTA_IMG_DESC: return "keine App-Kennung";
    default: return "falsches Projekt";
  }
}

// Gibt es etwas Neues? Beide Kennungen müssen gültige ELF-SHA-256 sein und sich unterscheiden (ein kaputtes Manifest meldet nie "Update").
static inline bool ota_needs_update(const char *running_elf, const char *manifest_elf) {
  return ota_hex_ok(running_elf, 64) && ota_hex_ok(manifest_elf, 64) && strcmp(running_elf, manifest_elf) != 0;
}

// Hinweis anzeigen? Nur wenn das Display wach ist (nicht dunkel/gedimmt: ein Tipp in ein dunkles Display darf nie zustimmen)
// und die Schlummerzeit (nach "Später" eine Stunde) um ist.
constexpr uint32_t OTA_NAG_MS = 3600000UL;
static inline bool ota_notice_due(uint32_t now_ms, uint32_t snooze_until_ms, bool awake) {
  if (!awake) return false;
  return snooze_until_ms == 0 || (int32_t)(now_ms - snooze_until_ms) >= 0;
}

static inline int ota_progress_pct(uint32_t done, uint32_t total) {
  if (!total) return 0;
  uint64_t p = (uint64_t)done * 100u / total;
  return p > 100 ? 100 : (int)p;
}

// Zeile "Firmware" auf der Status-Seite (Tippen sucht sofort nach einem Update).
// state: 0 IDLE, 1 AVAILABLE, 2 DOWNLOADING, 3 VERIFYING, 4 RESTARTING, 5 FAILED (Reihenfolge von ota_pull::State).
// busy = eine Suche läuft oder ist angefordert. Rückgabe: 0 neutral (grau), 1 gut (grün), 2 Problem (rot).
static inline int ota_status_line(int state, int pct, bool busy, uint32_t last_check_ms, bool last_failed, uint32_t now_ms, char *out, size_t n) {
  if (state == 2) { snprintf(out, n, "Lädt %d %%", pct); return 0; }
  if (state == 3) { snprintf(out, n, "Prüft die Datei …"); return 0; }
  if (state == 4) { snprintf(out, n, "Neustart …"); return 0; }
  if (busy) { snprintf(out, n, "Prüfe …"); return 0; }
  if (state == 5) { snprintf(out, n, "Update fehlgeschlagen"); return 2; }
  if (state == 1) { snprintf(out, n, "Update bereit – tippen"); return 0; }
  if (last_check_ms == 0) { snprintf(out, n, "Tippen: Update suchen"); return 0; }
  if (last_failed) { snprintf(out, n, "Abruf fehlgeschlagen"); return 2; }
  const uint32_t min = (now_ms - last_check_ms) / 60000UL;
  if (min < 1) snprintf(out, n, "Aktuell (gerade geprüft)");
  else if (min < 60) snprintf(out, n, "Aktuell (vor %u min)", (unsigned)min);
  else snprintf(out, n, "Aktuell (vor %u h)", (unsigned)(min / 60));
  return 1;
}
