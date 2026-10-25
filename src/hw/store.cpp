#include "store.h"
#include <Preferences.h>
#include <string.h>

static constexpr const char *kNs = "buddy";
static constexpr uint8_t kVersion = 1;

void store_defaults(Persist &p) {
  p = Persist{};
  p.needs = Needs{60, 60, 80};
  p.epoch = 0;
  p.brightness = 2;
  p.place[0] = '\0';
  p.lat = p.lon = 0;
  p.has_place = false;
}

bool store_load(Persist &p) {
  store_defaults(p);
  Preferences prefs;
  if (!prefs.begin(kNs, true)) return false;   // Namensraum fehlt = noch nie gespeichert
  if (prefs.getUChar("ver", 0) != kVersion) { prefs.end(); return false; }
  p.needs.affection = prefs.getFloat("aff", p.needs.affection);
  p.needs.fun = prefs.getFloat("fun", p.needs.fun);
  p.needs.energy = prefs.getFloat("ene", p.needs.energy);
  p.epoch = prefs.getULong("epoch", 0);
  p.brightness = prefs.getUChar("bright", 2);
  if (p.brightness > 2) p.brightness = 2;
  prefs.getString("place", p.place, sizeof p.place);
  p.place[sizeof p.place - 1] = '\0';
  p.lat = prefs.getFloat("lat", 0);
  p.lon = prefs.getFloat("lon", 0);
  p.has_place = prefs.getBool("hasp", false);
  prefs.end();
  mood_clamp(p.needs);
  return true;
}

void store_save(const Persist &p) {
  Preferences prefs;
  if (!prefs.begin(kNs, false)) return;
  prefs.putUChar("ver", kVersion);
  prefs.putFloat("aff", p.needs.affection);
  prefs.putFloat("fun", p.needs.fun);
  prefs.putFloat("ene", p.needs.energy);
  prefs.putULong("epoch", p.epoch);
  prefs.putUChar("bright", p.brightness);
  prefs.putString("place", p.place);
  prefs.putFloat("lat", p.lat);
  prefs.putFloat("lon", p.lon);
  prefs.putBool("hasp", p.has_place);
  prefs.end();
}
