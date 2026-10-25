#pragma once
#include <stdint.h>
#include "claude_state.h"
#include "mood.h"

// Claude-Modus: welches Gefühl und welche Zusatzeffekte der Buddy zeigt. Reine Logik (Host-Test test/test_claude_face).
// Spec: docs/superpowers/specs/2026-10-25-claude-modus-design.md
enum ClaudeFx : uint8_t {
  CFX_NONE = 0,
  CFX_STRAIGHT = 1,   // Blick gerade auf den Betrachter (Pupillen Mitte, pupil_r 18, Brauen hoch)
  CFX_JUMP = 2,       // Pupillen springen alle 250 ms links <-> rechts (Lesen)
  CFX_TYPING = 4,     // Overlay OV_TYPING (Schreiben)
  CFX_RING = 8,       // drehendes Ring-Segment in Claude-Orange (Ausfuehren)
  CFX_HAND = 16       // Hand (Fertig)
};
struct ClaudeFace { Emotion emo; uint8_t fx; };

// Zustand mit eigenem Gesicht (nicht OFF, nicht IDLE).
inline bool claude_busy(ClaudeState cs) { return cs != ClaudeState::OFF && cs != ClaudeState::IDLE; }

// base = Gefuehl des Haustiers (Beduerfnisse, Beruehrung, Pomodoro-FOCUSED, ggf. SLEEPING).
// Vorrang: Pomodoro-Arbeit > Beruehrungs-Gefuehl (touch_active, vom Aufrufer auf 3 s begrenzt) > Claude-Gesicht > Haustier.
// Auch ein schlafendes Haustier (Nickerchen, Dunkel) bekommt waehrend Claude arbeitet das Claude-Gesicht.
inline ClaudeFace claude_face(ClaudeState cs, bool pomo_work, bool touch_active, Emotion base) {
  if (!claude_busy(cs) || pomo_work || touch_active) return ClaudeFace{base, CFX_NONE};
  switch (cs) {
    case ClaudeState::LISTENING: return ClaudeFace{Emotion::SURPRISED, CFX_NONE};
    case ClaudeState::THINKING:  return ClaudeFace{Emotion::THINKING, CFX_NONE};
    case ClaudeState::READING:   return ClaudeFace{Emotion::THINKING, CFX_JUMP};
    case ClaudeState::CODING:    return ClaudeFace{Emotion::FOCUSED, CFX_TYPING};
    case ClaudeState::RUNNING:   return ClaudeFace{Emotion::FOCUSED, CFX_RING};
    case ClaudeState::FAILED:    return ClaudeFace{Emotion::SCARED, CFX_NONE};
    case ClaudeState::NEEDS_YOU: return ClaudeFace{Emotion::CONTENT, CFX_STRAIGHT};
    case ClaudeState::DONE:      return ClaudeFace{Emotion::EXCITED, CFX_HAND};
    default:                     return ClaudeFace{base, CFX_NONE};
  }
}
