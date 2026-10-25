#include "claude_api.h"
#include <Arduino.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <string.h>
#include "net.h"
#include "../hw/store.h"

static constexpr const char *kHost = "cyd-buddy";
static constexpr size_t kMaxBody = 256;
static constexpr size_t kKeyLen = 12;

static WebServer s_srv(80);
static ClaudeLogic *s_logic = nullptr;
static char s_key[kKeyLen + 1] = "";
static bool s_running = false;
static bool s_mdns = false;

static void make_key() {
  snprintf(s_key, sizeof s_key, "%08x%04x", (unsigned)esp_random(), (unsigned)(esp_random() & 0xFFFF));
}

// Laenge + Inhalt vergleichen, ohne beim ersten Unterschied abzubrechen.
static bool key_ok(const String &got) {
  if (got.length() != kKeyLen) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < kKeyLen; i++) diff |= (uint8_t)(got[i] ^ s_key[i]);
  return diff == 0;
}

// POST /claude als eigener RequestHandler mit "raw"-Modus des WebServers: Der Kern parst Anfragezeile und Header, ruft dann
// raw(RAW_START) auf, BEVOR irgendein Körperbyte gelesen wird (WebServer/Parsing.cpp: canRaw/raw-Zweig). Dort prüfen wir
// Schlüssel und Content-Length; bei Fehler antworten wir sofort (401/413) und schließen die Verbindung, so dass der Kern
// den Körper nie liest. Der erlaubte Körper (<= 256 B) landet in einem statischen Puffer, nicht auf dem Heap.
static char s_body[kMaxBody];
static size_t s_blen = 0;
static int s_rej = 0;            // 0 = angenommen, sonst bereits gesendeter Fehlercode
static bool s_raw_seen = false;  // raw() wurde aufgerufen (nicht bei multipart/form-data: dort liest der Kern den Körper vorab)
static bool s_raw_done = false;  // RAW_END erreicht, Körper vollständig

static void reject_early(WebServer &s, int code, const char *msg) {
  s_rej = code;
  s.setContentLength(CONTENT_LENGTH_NOT_SET);   // der Kern setzt das sonst erst NACH dem Parsen; beim ersten Aufruf stünde 0 darin
  s.send(code, "text/plain", msg);   // "Connection: close"
  s.client().clear();                // schon eingetroffene Bytes verwerfen (kein Heap)
  s.client().stop();                 // Kern bricht danach die Körperschleife ab (read() < 0 -> RAW_ABORTED)
}

class ClaudePostHandler : public RequestHandler {
 public:
  using RequestHandler::canHandle;
  bool canHandle(WebServer &, HTTPMethod m, const String &uri) override {
    s_blen = 0; s_rej = 0; s_raw_seen = false; s_raw_done = false;   // Beginn einer neuen Anfrage
    return m == HTTP_POST && uri == "/claude";
  }
  using RequestHandler::canRaw;
  bool canRaw(WebServer &, const String &uri) override { return uri == "/claude"; }
  void raw(WebServer &s, const String &, HTTPRaw &r) override {
    switch (r.status) {
      case RAW_START: {
        s_raw_seen = true;
        const int len = s.clientContentLength();
        if (!key_ok(s.header("X-Buddy-Key"))) reject_early(s, 401, "unauthorized");
        else if (len < 0 || len > (int)kMaxBody) reject_early(s, 413, "too large");
        break;
      }
      case RAW_WRITE:
        if (s_rej == 0 && s_blen + r.currentSize <= kMaxBody) { memcpy(s_body + s_blen, r.buf, r.currentSize); s_blen += r.currentSize; }
        break;
      case RAW_END: s_raw_done = (s_rej == 0); break;
      default: break;
    }
  }
  using RequestHandler::handle;
  bool handle(WebServer &s, HTTPMethod, const String &) override {
    if (s_rej) return true;   // schon beantwortet, Verbindung zu
    if (!s_raw_seen) {        // multipart/form-data: der Kern hat den Körper schon gelesen; Schlüssel trotzdem prüfen, Inhalt nicht annehmen
      if (!key_ok(s.header("X-Buddy-Key"))) s.send(401, "text/plain", "unauthorized");
      else s.send(415, "text/plain", "unsupported content type");
      return true;
    }
    if (!s_raw_done || s_blen == 0) { s.send(400, "text/plain", "no json body"); return true; }
    JsonDocument doc;
    if (deserializeJson(doc, s_body, s_blen) || !doc.is<JsonObject>()) { s.send(400, "text/plain", "bad json"); return true; }
    const char *e = doc["e"] | "";
    const char *t = doc["t"] | "";
    const char *n = doc["n"] | "";
    const char *sid = doc["sid"] | "";
    const bool interrupt = doc["i"].as<int>() != 0;
    if (s_logic) claude_event(*s_logic, e, t, n, sid, interrupt, millis());
    s.send(204);
    return true;
  }
};
static ClaudePostHandler s_post_handler;

static void on_ping() {
  if (!key_ok(s_srv.header("X-Buddy-Key"))) { s_srv.send(401, "text/plain", "unauthorized"); return; }
  char buf[64];
  snprintf(buf, sizeof buf, "{\"ok\":true,\"state\":\"%s\"}", claude_state_name(claude_api_state(millis())));
  s_srv.send(200, "application/json", buf);
}

static void start_server() {
  // Handler und Header-Liste nur beim ersten Start: collectHeaders() hängt bei jedem Aufruf an (_headerKeysCount += n),
  // on()/addHandler() hängen Handler an; stop()/begin() lässt beides stehen.
  static bool registered = false;
  if (!registered) {
    static const char *kHeaders[] = {"X-Buddy-Key"};
    s_srv.collectHeaders(kHeaders, 1);
    s_srv.addHandler(&s_post_handler);
    s_srv.on("/claude/ping", HTTP_GET, on_ping);
    registered = true;
  }
  s_srv.begin();
  s_mdns = MDNS.begin(kHost);
  if (s_mdns) MDNS.addService("http", "tcp", 80);
  s_running = true;
  Serial.printf("claude: Empfaenger offen (%s.local, Port 80, mDNS %s)\n", kHost, s_mdns ? "ok" : "Fehler");
}

void claude_api_stop() {
  if (!s_running) return;
  s_srv.stop();
  if (s_mdns) MDNS.end();
  s_mdns = false;
  s_running = false;
}

void claude_api_begin(ClaudeLogic *logic) {
  s_logic = logic;
  if (!store_load_claude_key(s_key)) {
    make_key();
    store_save_claude_key(s_key);
  }
  Serial.printf("claude: Schlüssel %s\n", s_key);
}

void claude_api_loop() {
  if (!s_logic) return;
  const bool ok = net_wifi_ok() && !net_portal_active();   // das WLAN-Portal braucht Port 80
  if (ok && !s_running) start_server();
  else if (!ok && s_running) claude_api_stop();
  if (s_running) s_srv.handleClient();
}

const char *claude_api_key() { return s_key; }

ClaudeState claude_api_state(uint32_t now_ms) { return s_logic ? claude_state(*s_logic, now_ms) : ClaudeState::OFF; }
