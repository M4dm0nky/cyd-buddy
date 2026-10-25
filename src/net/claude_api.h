#pragma once
#include <stdint.h>
#include "../logic/claude_state.h"

// Claude-Modus: kleiner HTTP-Empfaenger (Arduino WebServer, Port 80, mDNS "madbuddy").
// POST /claude (Header X-Buddy-Key, JSON <= 256 B) -> claude_event; GET /claude/ping -> {"ok":true,"state":"..."}.
// Laeuft komplett im Haupt-Task (handleClient() aus claude_api_loop), kein eigener Thread.
void claude_api_begin(ClaudeLogic *logic);   // Schluessel laden/erzeugen (NVS "ckey"), seriell ausgeben; Server startet spaeter
void claude_api_loop();                      // aus app_loop: startet/stoppt den Server je nach WLAN, bedient Anfragen
void claude_api_stop();                      // Port 80 freigeben (vor dem WLAN-Portal von WiFiManager)
const char *claude_api_key();                // 12 Hex-Zeichen, "" vor claude_api_begin
ClaudeState claude_api_state(uint32_t now_ms);   // aktueller Claude-Zustand (OFF ohne Logik)
