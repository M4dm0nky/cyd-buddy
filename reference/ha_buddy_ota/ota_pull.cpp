#include "ota_pull.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <esp_app_desc.h>
#include <esp_ota_ops.h>
#include <esp_heap_caps.h>
#include <mbedtls/sha256.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include "../net/net.h"
#include "../ha/ha_ws.h"
#include "../config/secrets.h"
#include "../util/psram_alloc.h"
#include "../logic/ota_policy.h"
#include "../diag/diag.h"

// Optional in secrets.h: SECRET_OTA_BASE_URL (Ordner mit manifest.json und der .bin, ohne Schrägstrich am Ende) und SECRET_OTA_TOKEN.
// Ohne beides: Home Assistant selbst, Medienordner, mit dem HA-Token (Bearer). Die .bin enthält WLAN-Zugang und HA-Token -- sie darf
// nie unter einer Adresse ohne Anmeldung liegen (also NICHT in HAs /local/ bzw. www).
#ifndef SECRET_OTA_BASE_URL
#define SECRET_OTA_BASE_URL ""
#endif
#ifndef SECRET_OTA_TOKEN
#define SECRET_OTA_TOKEN ""
#endif

namespace ota_pull {
namespace {
// Projektname im Image: beim Waveshare-Boardpaket "arduino-lib-builder" (nicht der Sketchname) -- darum der Name der laufenden Firmware als Soll.
const char *project_name() { return esp_app_get_description()->project_name; }
constexpr uint32_t kFirstPollMs = 90UL * 1000UL;
constexpr uint32_t kPollMs = 10UL * 60UL * 1000UL;
constexpr uint32_t kRetryMs = 3UL * 60UL * 1000UL;
constexpr uint32_t kMinSize = 200UL * 1024UL;
constexpr size_t kChunk = 4096;
constexpr uint32_t kStallMs = 15000;
constexpr uint32_t kHealthAfterMs = 20000;   // so lange muss die neue Firmware laufen, bevor sie als gut gilt

portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
Info g_info = {};
volatile bool g_consent = false;
volatile bool g_check_now = false, g_fetching = false;   // Suche angefordert (Status-Seite) / läuft gerade
uint32_t g_check_req_ms = 0;
volatile bool g_ui_alive = false;
bool g_marked = false;
uint32_t g_next_poll = 0;
char g_running_elf[65] = "";
char g_slot[17] = "?", g_build[24] = "";
int g_img_state = -1;
char g_man_elf[65] = "", g_man_sha[65] = "", g_man_file[48] = "";
uint32_t g_man_size = 0;

void set_state(State s) { portENTER_CRITICAL(&g_mux); g_info.state = s; portEXIT_CRITICAL(&g_mux); }
void set_error(const char *e) {
  portENTER_CRITICAL(&g_mux);
  strlcpy(g_info.error, e, sizeof g_info.error);
  g_info.state = FAILED;
  portEXIT_CRITICAL(&g_mux);
  char n[40];
  snprintf(n, sizeof n, "OTA fehlgeschlagen: %.22s", e);
  diag_event(n, 0);
  Serial.printf("ota: fehlgeschlagen: %s\n", e);
}

void hex_of(const uint8_t *b, size_t n, char *out) {
  static const char k[] = "0123456789abcdef";
  for (size_t i = 0; i < n; i++) { out[2 * i] = k[b[i] >> 4]; out[2 * i + 1] = k[b[i] & 15]; }
  out[2 * n] = '\0';
}

// Dateiname aus dem Manifest: nur Buchstaben, Ziffern, Punkt, Minus, Unterstrich (kein Pfad, keine Umlenkung der URL)
bool file_name_ok(const char *f) {
  size_t n = f ? strlen(f) : 0;
  if (n == 0 || n >= sizeof g_man_file || f[0] == '.') return false;
  for (size_t i = 0; i < n; i++) {
    const char c = f[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')) return false;
  }
  return true;
}

void make_url(const char *file, char *out, size_t n) {
  if (SECRET_OTA_BASE_URL[0]) snprintf(out, n, "%s/%s", SECRET_OTA_BASE_URL, file);
  else snprintf(out, n, "http://%s:%u/media/local/ha_buddy/%s", net::config().ha_host, (unsigned)net::config().ha_port, file);
}

void add_auth(HTTPClient &http) {
  const char *tok = SECRET_OTA_TOKEN[0] ? SECRET_OTA_TOKEN : net::config().ha_token;
  http.addHeader("Authorization", String("Bearer ") + tok);
}

void record_check(bool ok) {
  portENTER_CRITICAL(&g_mux);
  g_info.last_check_ms = millis() ? millis() : 1;
  g_info.last_failed = !ok;
  portEXIT_CRITICAL(&g_mux);
}

// true = Manifest gelesen; *newer sagt, ob es etwas Neues gibt
bool fetch_manifest(bool *newer) {
  *newer = false;
  char url[200];
  make_url("manifest.png", url, sizeof url);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(8000);
  http.setConnectTimeout(3000);
  if (!http.begin(url)) return false;
  add_auth(http);
  const int code = http.GET();
  if (code != 200) { Serial.printf("ota: Manifest HTTP %d\n", code); http.end(); return false; }
  JsonDocument doc(psram_allocator());
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) { Serial.printf("ota: Manifest-JSON %s\n", err.c_str()); return false; }
  const char *proj = doc["project"] | "";
  const char *elf = doc["elf"] | "";
  const char *sha = doc["sha256"] | "";
  const char *file = doc["file"] | "";
  const uint32_t size = doc["size"] | 0u;
  if (strcmp(proj, project_name()) != 0 || !ota_hex_ok(elf, 64) || !ota_hex_ok(sha, 64) || !file_name_ok(file) || size < kMinSize) {
    Serial.println("ota: Manifest unbrauchbar");
    return false;
  }
  strlcpy(g_man_elf, elf, sizeof g_man_elf);
  strlcpy(g_man_sha, sha, sizeof g_man_sha);
  strlcpy(g_man_file, file, sizeof g_man_file);
  g_man_size = size;
  *newer = ota_needs_update(g_running_elf, g_man_elf);
  if (*newer) {
    portENTER_CRITICAL(&g_mux);
    strlcpy(g_info.built, doc["built"] | "", sizeof g_info.built);
    g_info.size = size;
    g_info.pct = 0;
    g_info.error[0] = '\0';
    g_info.state = AVAILABLE;
    portEXIT_CRITICAL(&g_mux);
    char n[40];
    snprintf(n, sizeof n, "OTA: Update bereit %.20s", (const char *)(doc["built"] | ""));
    diag_event(n, 2);
  }
  return true;
}

// Blockiert bis zum Ende. Bei Erfolg Neustart, bei Fehler FAILED.
void download() {
  char url[200];
  make_url(g_man_file, url, sizeof url);
  set_state(DOWNLOADING);
  portENTER_CRITICAL(&g_mux); g_info.pct = 0; portEXIT_CRITICAL(&g_mux);

  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(15000);
  http.setConnectTimeout(4000);
  if (!http.begin(url)) { set_error("URL"); return; }
  add_auth(http);
  const int code = http.GET();
  if (code != 200) {
    char e[40]; snprintf(e, sizeof e, "HTTP %d", code);
    http.end(); set_error(e); return;
  }
  if ((uint32_t)http.getSize() != g_man_size) { http.end(); set_error("Groesse passt nicht"); return; }

  uint8_t *buf = (uint8_t *)heap_caps_malloc(kChunk, MALLOC_CAP_SPIRAM);
  if (!buf) { http.end(); set_error("kein Speicher"); return; }
  WiFiClient *s = http.getStreamPtr();

  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);

  uint32_t done = 0, last_data = millis();
  bool started = false, fail = false;
  char reason[40] = "";
  size_t have = 0;   // Bytes im Puffer, die noch nicht geschrieben sind (nur vor dem ersten Schreiben nötig)
  while (done < g_man_size && !fail) {
    const int avail = s->available();
    if (avail <= 0) {
      if (!http.connected() && !s->available()) { strlcpy(reason, "Verbindung weg", sizeof reason); fail = true; }
      else if (millis() - last_data > kStallMs) { strlcpy(reason, "Zeitueberschreitung", sizeof reason); fail = true; }
      else vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }
    size_t want = kChunk - have;
    if (want > (size_t)avail) want = avail;
    if (want > g_man_size - done) want = g_man_size - done;
    const int r = s->read(buf + have, want);
    if (r <= 0) { vTaskDelay(pdMS_TO_TICKS(2)); continue; }
    last_data = millis();
    have += r;
    if (!started) {
      if (have < OTA_IMG_CHECK_BYTES) continue;   // erst prüfen, wenn der Anfang da ist
      char elf[65];
      const OtaImg chk = ota_image_check(buf, have, project_name(), elf);
      if (chk != OTA_IMG_OK) { strlcpy(reason, ota_image_error(chk), sizeof reason); fail = true; break; }
      if (strcmp(elf, g_man_elf) != 0) { strlcpy(reason, "Image passt nicht zum Manifest", sizeof reason); fail = true; break; }
      if (!Update.begin(g_man_size, U_FLASH)) { snprintf(reason, sizeof reason, "Update.begin: %.24s", Update.errorString()); fail = true; break; }
      started = true;
    }
    mbedtls_sha256_update(&sha, buf, have);
    if (Update.write(buf, have) != have) { snprintf(reason, sizeof reason, "Schreiben: %.24s", Update.errorString()); fail = true; break; }
    done += have;
    have = 0;
    portENTER_CRITICAL(&g_mux); g_info.pct = (uint8_t)ota_progress_pct(done, g_man_size); portEXIT_CRITICAL(&g_mux);
    vTaskDelay(1);   // der Task läuft mit Idle-Priorität: dem Idle-Task Zeit lassen (Watchdog)
  }
  http.end();
  free(buf);

  uint8_t digest[32];
  mbedtls_sha256_finish(&sha, digest);
  mbedtls_sha256_free(&sha);
  if (!fail && done != g_man_size) { strlcpy(reason, "unvollstaendig", sizeof reason); fail = true; }
  if (!fail) {
    set_state(VERIFYING);
    char got[65];
    hex_of(digest, 32, got);
    if (strcmp(got, g_man_sha) != 0) { strlcpy(reason, "SHA-256 falsch", sizeof reason); fail = true; }
  }
  if (fail) {
    if (started) Update.abort();
    set_error(reason[0] ? reason : "Fehler");
    return;
  }
  if (!Update.end(true)) {
    char e[40]; snprintf(e, sizeof e, "Update.end: %.24s", Update.errorString());
    set_error(e);
    return;
  }
  diag_event("OTA: geschrieben, Neustart", 1);
  set_state(RESTARTING);
  Serial.println("ota: ok, Neustart in 2 s");
  vTaskDelay(pdMS_TO_TICKS(2000));
  ESP.restart();
}
}  // namespace

void begin() {
  const esp_app_desc_t *d = esp_app_get_description();
  hex_of(d->app_elf_sha256, 32, g_running_elf);
  g_next_poll = kFirstPollMs;
  const esp_partition_t *run = esp_ota_get_running_partition();
  esp_ota_img_states_t st = ESP_OTA_IMG_UNDEFINED;
  if (run) esp_ota_get_state_partition(run, &st);
  if (run) snprintf(g_slot, sizeof g_slot, "%s", run->label);
  g_img_state = (int)st;
  snprintf(g_build, sizeof g_build, "%.11s %.11s", d->date, d->time);
  Serial.printf("ota: Partition %s, Zustand %d, ELF %.12s\n", run ? run->label : "?", (int)st, g_running_elf);
  char n[40];
  snprintf(n, sizeof n, "OTA: %s Zustand %d", run ? run->label : "?", (int)st);
  diag_event(n, 2);
  // Bleibt der Zustand PENDING_VERIFY (Bootloader mit Rollback), gilt das Image erst nach dem Gesundheitsnachweis in loop().
  g_marked = (st != ESP_OTA_IMG_PENDING_VERIFY);
}

void loop() {
  const uint32_t now = millis();

  // Gesundheitsnachweis nach einem Update: UI-Takt läuft, HA verbunden, Mindestlaufzeit. Sonst bootet der Bootloader beim nächsten Reset zurück.
  if (!g_marked && g_ui_alive && ha::connected() && now > kHealthAfterMs) {
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) diag_event("OTA: neue Firmware als gut bestaetigt", 1);
    g_marked = true;
  }

  const State st = info().state;
  if (st == AVAILABLE && g_consent) {
    g_consent = false;
    // Zustimmung gilt der Firmware, die auf dem Display stand; seit dem Manifest kann eine neuere veröffentlicht worden sein -> frisch lesen
    bool newer = false;
    if (!fetch_manifest(&newer)) { set_error("Manifest nicht lesbar"); return; }
    if (!newer) { set_error("Update nicht mehr noetig"); return; }
    download();
    g_next_poll = millis() + kPollMs;
    return;
  }
  g_consent = false;
  if (st == DOWNLOADING || st == VERIFYING || st == RESTARTING) return;
  if (g_check_now) {   // Status-Seite: "jetzt suchen"; hebt Schlummern und eine alte Fehlermeldung auf
    g_check_now = false;
    if (!net::wifi_ok()) { record_check(false); return; }
    portENTER_CRITICAL(&g_mux);
    g_info.snooze_until = 0;
    if (g_info.state == FAILED) { g_info.state = IDLE; g_info.error[0] = '\0'; }
    portEXIT_CRITICAL(&g_mux);
    g_fetching = true;
    bool newer = false;
    const bool ok = fetch_manifest(&newer);
    if (ok && !newer && info().state == AVAILABLE) set_state(IDLE);   // inzwischen zurückgenommen/gleich der laufenden Firmware
    g_fetching = false;
    record_check(ok);
    g_next_poll = millis() + (ok ? kPollMs : kRetryMs);
    return;
  }
  if (st == AVAILABLE) return;   // wartet auf Zustimmung; kein erneutes Abfragen nötig
  if ((int32_t)(now - g_next_poll) < 0) return;
  if (!net::wifi_ok()) return;
  g_fetching = true;
  bool newer = false;
  const bool ok = fetch_manifest(&newer);
  g_fetching = false;
  record_check(ok);
  g_next_poll = millis() + (ok ? kPollMs : kRetryMs);
}

Info info() {
  Info i;
  portENTER_CRITICAL(&g_mux);
  i = g_info;
  portEXIT_CRITICAL(&g_mux);
  return i;
}

void consent() {
  if (info().state == AVAILABLE) g_consent = true;
}

void check_now() {
  const State st = info().state;
  if (st == DOWNLOADING || st == VERIFYING || st == RESTARTING) return;
  g_check_req_ms = millis();
  g_check_now = true;
}

bool check_busy() { return g_fetching || (g_check_now && millis() - g_check_req_ms < 60000UL); }

void snooze() {
  portENTER_CRITICAL(&g_mux);
  g_info.snooze_until = millis() + OTA_NAG_MS;
  if (!g_info.snooze_until) g_info.snooze_until = 1;
  portEXIT_CRITICAL(&g_mux);
}

void dismiss_failure() {
  portENTER_CRITICAL(&g_mux);
  if (g_info.state == FAILED) { g_info.state = IDLE; g_info.error[0] = '\0'; }
  portEXIT_CRITICAL(&g_mux);
  g_next_poll = millis() + kRetryMs;
}

void ui_alive() { g_ui_alive = true; }
const char *slot() { return g_slot; }
int image_state() { return g_img_state; }
const char *elf12() { static char e[13]; memcpy(e, g_running_elf, 12); e[12] = '\0'; return e; }
const char *build() { return g_build; }
}  // namespace ota_pull
