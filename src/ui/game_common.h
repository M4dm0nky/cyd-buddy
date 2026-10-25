#pragma once
#include <stdint.h>
#include "../hw/display.h"

// Gemeinsames für die Spiel-Bildschirme: Spielfeld 320x200 unter einem 40-px-Kopf, Ergebnis-Anzeige.
enum class GameResult : uint8_t { RUNNING, WON, LOST };
constexpr int GAME_TOP = 40;                 // Spielfeld beginnt hier (darüber Restzeit/Fortschritt)
constexpr uint32_t GAME_RESULT_MS = 2000;    // so lange bleibt das Ergebnis stehen

void game_show_result(LGFX &lcd, bool won);  // großes Augenpaar: gewonnen EXCITED, verloren BORED
