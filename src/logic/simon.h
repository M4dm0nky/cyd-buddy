#pragma once
#include <stdint.h>
#include "eye_model.h"

// Spiel „Simon“: Folge aus 4 Feldern wird länger; Länge 6 geschafft = gewonnen. Reine Logik (Host-Test test/test_games).
// AWAIT_INPUT statt INPUT: Arduino definiert INPUT als Makro.
enum class SimonState : uint8_t { SHOW, AWAIT_INPUT, WON, LOST };
constexpr uint8_t SIMON_WIN_LEN = 6;
constexpr uint8_t SIMON_FIELDS = 4;
struct Simon { uint8_t seq[SIMON_WIN_LEN]; uint8_t len, pos; uint32_t seed; SimonState state; };

// Füllt seq[0..5] mit 0..3, Länge 1, Zustand SHOW.
inline void simon_start(Simon &s, uint32_t seed) {
  s.seed = seed ? seed : 1;
  for (uint8_t i = 0; i < SIMON_WIN_LEN; i++) s.seq[i] = (uint8_t)rng_range(s.seed, 0, SIMON_FIELDS - 1);
  s.len = 1;
  s.pos = 0;
  s.state = SimonState::SHOW;
}
// Anzeige der Folge fertig: jetzt tippt der Spieler (AWAIT_INPUT).
inline void simon_shown(Simon &s) {
  if (s.state != SimonState::SHOW) return;
  s.state = SimonState::AWAIT_INPUT;
  s.pos = 0;
}
// Nur in AWAIT_INPUT. Richtig und letztes Feld: Länge 6 → WON, sonst nächste Runde (len++, SHOW). Falsch: LOST.
inline void simon_press(Simon &s, uint8_t field) {
  if (s.state != SimonState::AWAIT_INPUT) return;
  if (field != s.seq[s.pos]) { s.state = SimonState::LOST; return; }
  s.pos++;
  if (s.pos < s.len) return;
  if (s.len == SIMON_WIN_LEN) { s.state = SimonState::WON; return; }
  s.len++;
  s.pos = 0;
  s.state = SimonState::SHOW;
}
