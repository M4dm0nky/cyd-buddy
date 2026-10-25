#pragma once
#include <stdint.h>
#include <string.h>

// Claude-Modus: Zustand aus den Hook-Ereignissen von Claude Code. Reine Logik, Zeit kommt als Parameter
// (Host-Test test/test_claude_state). Spec: docs/superpowers/specs/2026-10-25-claude-modus-design.md
enum class ClaudeState : uint8_t { OFF, IDLE, LISTENING, THINKING, READING, CODING, RUNNING, FAILED, NEEDS_YOU, DONE };

constexpr uint32_t CLAUDE_TIMEOUT_MS = 15UL * 60UL * 1000UL;       // kein Ereignis -> OFF
constexpr uint32_t CLAUDE_NEEDS_MAX_MS = 30UL * 60UL * 1000UL;     // NEEDS_YOU hoechstens so lange
constexpr uint32_t CLAUDE_LISTENING_MS = 1200;                     // LISTENING -> THINKING
constexpr uint32_t CLAUDE_FAILED_MS = 3000;                        // FAILED -> THINKING
constexpr uint32_t CLAUDE_DONE_MS = 8000;                          // DONE -> IDLE
constexpr uint32_t CLAUDE_NEEDS_PRE_IGNORE_MS = 1500;             // PreToolUse hebt NEEDS_YOU erst danach auf
constexpr size_t CLAUDE_SID_LEN = 8;

struct ClaudeLogic {
  ClaudeState base = ClaudeState::OFF;   // zuletzt gesetzter Zustand (ohne Zeitablauf)
  uint32_t since = 0;                    // seit wann base gilt
  uint32_t last_event_ms = 0;
  bool has_event = false;
  bool needs_open = false;               // NEEDS_YOU einer Sitzung ist offen
  char needs_sid[CLAUDE_SID_LEN + 1] = "";
  uint8_t helpers = 0;                   // laufende Helfer (Subagents)
};

namespace claude_detail {
inline bool eq(const char *a, const char *b) { return a && b && strcmp(a, b) == 0; }
inline bool in(const char *s, const char *const *list, size_t n) {
  for (size_t i = 0; i < n; i++) if (eq(s, list[i])) return true;
  return false;
}
inline bool same_sid(const char *a, const char *b) {
  return strncmp(a ? a : "", b ? b : "", CLAUDE_SID_LEN) == 0;
}
inline ClaudeState tool_state(const char *tool) {
  static const char *const kRead[] = {"Read", "Grep", "Glob", "WebSearch", "WebFetch", "ToolSearch", "LS"};
  static const char *const kCode[] = {"Edit", "Write", "NotebookEdit", "MultiEdit"};
  if (in(tool, kRead, sizeof kRead / sizeof *kRead)) return ClaudeState::READING;
  if (in(tool, kCode, sizeof kCode / sizeof *kCode)) return ClaudeState::CODING;
  return ClaudeState::RUNNING;   // Bash, PowerShell und alles Unbekannte
}
inline bool needs_notification(const char *n) {
  static const char *const kN[] = {"permission_prompt", "idle_prompt", "elicitation_dialog", "agent_needs_input"};
  return in(n, kN, sizeof kN / sizeof *kN);
}
}  // namespace claude_detail

// Angezeigter Zustand zum Zeitpunkt now (Zeitablauf eingerechnet).
inline ClaudeState claude_state(const ClaudeLogic &L, uint32_t now) {
  if (!L.has_event || L.base == ClaudeState::OFF) return ClaudeState::OFF;
  if (L.base == ClaudeState::NEEDS_YOU) {
    return now - L.since >= CLAUDE_NEEDS_MAX_MS ? ClaudeState::OFF : ClaudeState::NEEDS_YOU;
  }
  if (now - L.last_event_ms >= CLAUDE_TIMEOUT_MS) return ClaudeState::OFF;
  const uint32_t age = now - L.since;
  switch (L.base) {
    case ClaudeState::LISTENING: return age >= CLAUDE_LISTENING_MS ? ClaudeState::THINKING : ClaudeState::LISTENING;
    case ClaudeState::FAILED:    return age >= CLAUDE_FAILED_MS ? ClaudeState::THINKING : ClaudeState::FAILED;
    case ClaudeState::DONE:      return age >= CLAUDE_DONE_MS ? ClaudeState::IDLE : ClaudeState::DONE;
    default:                     return L.base;
  }
}
inline bool claude_active(const ClaudeLogic &L, uint32_t now) { return claude_state(L, now) != ClaudeState::OFF; }
inline bool claude_needs_you(const ClaudeLogic &L, uint32_t now) { return claude_state(L, now) == ClaudeState::NEEDS_YOU; }
inline uint8_t claude_helpers(const ClaudeLogic &L) { return L.helpers; }
inline uint8_t claude_helpers(const ClaudeLogic &L, uint32_t now) { return claude_active(L, now) ? L.helpers : 0; }

inline const char *claude_state_name(ClaudeState s) {
  switch (s) {
    case ClaudeState::OFF: return "off";
    case ClaudeState::IDLE: return "idle";
    case ClaudeState::LISTENING: return "listening";
    case ClaudeState::THINKING: return "thinking";
    case ClaudeState::READING: return "reading";
    case ClaudeState::CODING: return "coding";
    case ClaudeState::RUNNING: return "running";
    case ClaudeState::FAILED: return "failed";
    case ClaudeState::NEEDS_YOU: return "needs_you";
    case ClaudeState::DONE: return "done";
  }
  return "off";
}

// Ein Hook-Ereignis verarbeiten. e = Hook-Name (z. B. "PreToolUse"), tool/ntype/sid duerfen null oder leer sein.
inline void claude_event(ClaudeLogic &L, const char *e, const char *tool, const char *ntype, const char *sid,
                         bool interrupt, uint32_t now) {
  using namespace claude_detail;
  if (!e) return;
  ClaudeState next = ClaudeState::OFF;
  bool set_state = true;
  if (eq(e, "UserPromptSubmit")) next = ClaudeState::LISTENING;
  else if (eq(e, "PostToolUse") || eq(e, "SessionStart")) next = ClaudeState::THINKING;
  else if (eq(e, "SubagentStop")) next = ClaudeState::THINKING;
  else if (eq(e, "SubagentStart")) set_state = false;
  else if (eq(e, "PreToolUse")) next = eq(tool, "AskUserQuestion") ? ClaudeState::NEEDS_YOU : tool_state(tool);
  else if (eq(e, "PostToolUseFailure")) next = interrupt ? ClaudeState::THINKING : ClaudeState::FAILED;
  else if (eq(e, "PermissionRequest")) next = ClaudeState::NEEDS_YOU;
  else if (eq(e, "Notification")) {
    if (!needs_notification(ntype)) return;   // andere Meldungen aendern nichts
    next = ClaudeState::NEEDS_YOU;
  } else if (eq(e, "Stop")) next = ClaudeState::DONE;
  else if (eq(e, "SessionEnd")) next = ClaudeState::OFF;
  else return;   // unbekanntes Ereignis

  // Abgelaufene Sitzung: alles vergessen, bevor das neue Ereignis gilt.
  const bool expired = L.has_event && (L.base == ClaudeState::NEEDS_YOU ? now - L.since >= CLAUDE_NEEDS_MAX_MS
                                                                       : now - L.last_event_ms >= CLAUDE_TIMEOUT_MS);
  if (expired) L = ClaudeLogic{};

  if (eq(e, "SubagentStart")) { if (L.helpers < 255) L.helpers++; }
  else if (eq(e, "SubagentStop")) { if (L.helpers > 0) L.helpers--; }
  else if (eq(e, "UserPromptSubmit") || eq(e, "Stop")) L.helpers = 0;   // alle Helfer gehören zur Runde

  const bool other_needs = L.needs_open && L.base == ClaudeState::NEEDS_YOU && !same_sid(L.needs_sid, sid);
  // Verspätete Ereignisse (Hooks laufen nicht streng der Reihe nach) dürfen DONE / NEEDS_YOU nicht überschreiben.
  bool late = false;
  if (claude_state(L, now) == ClaudeState::DONE) {
    late = !(eq(e, "UserPromptSubmit") || eq(e, "PreToolUse") || eq(e, "PermissionRequest") || eq(e, "Notification") ||
             eq(e, "SessionStart") || eq(e, "Stop") || eq(e, "SessionEnd"));
  } else if (L.needs_open && L.base == ClaudeState::NEEDS_YOU && !other_needs) {
    if (eq(e, "PostToolUse") || eq(e, "SubagentStop")) late = true;
    else if (eq(e, "PreToolUse") && next != ClaudeState::NEEDS_YOU && now - L.since < CLAUDE_NEEDS_PRE_IGNORE_MS) late = true;
  }
  L.last_event_ms = now;
  L.has_event = true;
  if (other_needs) return;   // NEEDS_YOU einer anderen Sitzung bleibt, bis von ihr ein Ereignis kommt
  if (late) return;
  if (!set_state) return;   // SubagentStart: nur der Zaehler
  if (next == ClaudeState::OFF) { L = ClaudeLogic{}; return; }
  L.base = next;
  L.since = now;
  L.needs_open = next == ClaudeState::NEEDS_YOU;
  if (L.needs_open) {
    strncpy(L.needs_sid, sid ? sid : "", CLAUDE_SID_LEN);
    L.needs_sid[CLAUDE_SID_LEN] = '\0';
  }
}
