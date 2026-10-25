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

static void on_post() {
  if (!key_ok(s_srv.header("X-Buddy-Key"))) { s_srv.send(401, "text/plain", "unauthorized"); return; }
  if (s_srv.clientContentLength() > (int)kMaxBody) { s_srv.send(413, "text/plain", "too large"); return; }
  if (!s_srv.hasArg("plain")) { s_srv.send(400, "text/plain", "no json body"); return; }   // Content-Type: application/json
  const String &body = s_srv.arg("plain");
  if (body.length() > kMaxBody) { s_srv.send(413, "text/plain", "too large"); return; }
  JsonDocument doc;
  if (deserializeJson(doc, body) || !doc.is<JsonObject>()) { s_srv.send(400, "text/plain", "bad json"); return; }
  const char *e = doc["e"] | "";
  const char *t = doc["t"] | "";
  const char *n = doc["n"] | "";
  const char *sid = doc["sid"] | "";
  const bool interrupt = doc["i"].as<int>() != 0;
  if (s_logic) claude_event(*s_logic, e, t, n, sid, interrupt, millis());
  s_srv.send(204);
}

static void on_ping() {
  if (!key_ok(s_srv.header("X-Buddy-Key"))) { s_srv.send(401, "text/plain", "unauthorized"); return; }
  char buf[64];
  snprintf(buf, sizeof buf, "{\"ok\":true,\"state\":\"%s\"}", claude_state_name(claude_api_state(millis())));
  s_srv.send(200, "application/json", buf);
}

static void start_server() {
  static const char *kHeaders[] = {"X-Buddy-Key"};
  s_srv.collectHeaders(kHeaders, 1);
  s_srv.on("/claude", HTTP_POST, on_post);
  s_srv.on("/claude/ping", HTTP_GET, on_ping);
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
