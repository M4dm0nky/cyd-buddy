#pragma once
#include <stdint.h>
#include "../logic/mood.h"

// Dauerhafte Werte im NVS (Preferences, Namensraum "buddy").
struct Persist {
  Needs needs;
  uint32_t epoch;        // letzte bekannte Unix-Zeit, 0 = unbekannt
  uint8_t brightness;    // 0..2
  char place[48];
  float lat, lon;
  bool has_place;        // lat/lon passen zu place
};

void store_defaults(Persist &p);     // needs {60,60,80}, epoch 0, brightness 2, place "", has_place false
bool store_load(Persist &p);         // false = nichts gespeichert (dann defaults)
void store_save(const Persist &p);
