# Fortschritt CYD-Buddy

Neueste Einträge oben. Format und Regeln: `docs/superpowers/plans/2026-10-25-cyd-buddy.md`, Abschnitt „Fortschritts-Protokoll“.

## Task 12 – OTA-Abruf portieren (ota_pull, ota_card) – ERLEDIGT
- Zeit: 2026-10-25 10:44 UTC
- Gemacht: `ota_pull.*` aus der Vorlage mit allen 9 Änderungen des Plans: Includes/diag entfernt (→ Serial), `kBase` GitHub-Release + `manifest.json`, neue `open_follow()` (Umleitungen selbst, ≤ 3 Sprünge, jede URL durch `ota_redirect_ok`, sonst „Umleitung abgelehnt“; TLS mit `useBuiltinCACertBundle()`), JsonDocument Standard-Allocator mit Filter, `malloc` statt PSRAM, Gesundheitsnachweis ohne HA, `project_name()` bleibt, `size > 1966080` → „zu gross fuer den Slot“, `loop()` über `net_task_hook`. `ota_card.*`: Karte „Update bereit“ (Ja/Später, Tipps < 1 s ignoriert), Fortschritt mit schmalen „konzentrierten“ Augen, Fehlerkarte. `app.cpp`: OTA-Bildschirm nur aus EYES bei wachem Display, `eyes_free()` bei DOWNLOADING, `eyes_begin()` danach, `ui_alive()` je Augenbild, Firmware-Zeile + „jetzt suchen“ in den Einstellungen, Update-Symbol im Statusstreifen.
- Dateien: src/ota/ota_pull.h, src/ota/ota_pull.cpp, src/ui/ota_card.h, src/ui/ota_card.cpp, src/app.cpp, docs/FORTSCHRITT.md (Zeitangaben korrigiert)
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,4 % (56 860 B), Flash 72,8 % (1 431 027 B); `ls -l .pio/build/cyd2usb/firmware.bin` → 1 459 792 B < 1 966 080 B. `pio test -e native` → 47 Tests, 0 Fehler
- Gegenprobe: –
- Nächster Schritt: Task 13 Step 1
- Offen/Fragen: (1) Zeitangaben der früheren Einträge waren geschätzt und falsch; jetzt auf die Commit-Zeiten korrigiert. (2) Plan nennt die Datei `src/net/net.cpp` (Hook) und `src/ui/settings.cpp` (Firmware-Zeile) als zu ändern: Der Hook war schon in Task 9 da, die Firmware-Zeile kommt über `settings_firmware_source()` aus `app.cpp` – beide Dateien blieben unverändert. (3) Speicher während des Manifest-Abrufs: TLS (~40–50 KB) läuft bei angelegtem Sprite (64 KB); am Gerät `min` im Heap-Log prüfen.
- Am Gerät zu prüfen: OTA Ende zu Ende mit zweitem Release (Karte, Später = 1 h Ruhe, Ja → Laden → Neustart, Fehlerkarte).

## Task 11 – Spiel-Bildschirme (Fang mich, Simon) – ERLEDIGT
- Zeit: 2026-10-25 10:41 UTC
- Gemacht: „Fang mich“ (kleines cyan Auge, alte Stelle schwarz übermalt, Restzeit-Balken + Treffer-Punkte oben) und „Simon“ (4 Felder rot/grün/blau/gelb, 500 ms hell / 200 ms Pause, Tipp leuchtet 150 ms). Spielfeld 320×200 unter 40-px-Kopf (`GAME_TOP`). Ende: 2 s großes Augenpaar (EXCITED bzw. BORED), dann EYES und `mood_event(GAME_WON/GAME_LOST)`. Seed aus `esp_random()`.
- Dateien: src/ui/game_common.*, src/ui/game_catch.*, src/ui/game_simon.*, src/app.cpp, src/logic/simon.h
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,2 % (56 428 B), Flash 72,2 % (1 420 259 B). `pio test -e native` → 47 Tests, 0 Fehler (8 Gruppen)
- Gegenprobe: –
- Nächster Schritt: Task 12 Step 1
- Offen/Fragen: **Abweichung:** `SimonState::INPUT` heißt jetzt `SimonState::AWAIT_INPUT`, weil Arduino `INPUT` als Makro definiert (Build-Fehler „expected identifier before numeric constant“). Der Test aus dem Plan benutzt `INPUT` nicht und läuft unverändert.
- Am Gerät zu prüfen: Spiele spielbar (Treffer-Toleranz, Tempo), Ergebnis-Augen.

## Task 10 – Menü, Uhr, Einstellungen – ERLEDIGT
- Zeit: 2026-10-25 10:38 UTC
- Gemacht: Menü 2×2 Kacheln (Spielen grün, Uhr gelb, Einstellungen violett, Schließen rot; 2. Ebene Fang-mich/Simon + Zurück), Uhr 15 s (Font7, Datum „So 25.10.“, Wetter, Augen schauen von unten herein; ohne Zeit „--:--“ + rotes WLAN), Einstellungen (Helligkeit 0→1→2→0, WLAN/Ort → Portal, Firmware-Zeile über Quelle ab Task 12, Info: Build-Datum, Heap, LDR). `app.cpp` mit `enum class Screen`; Stimmung/Licht laufen auf allen Bildschirmen weiter. Neu `logic/ascii.h` (UTF-8 → ASCII, die eingebauten Schriften haben keine Umlaute) mit Test.
- Dateien: src/ui/menu.*, src/ui/clock_view.*, src/ui/settings.*, src/ui/ui_common.*, src/logic/ascii.h, test/test_ascii/test_main.cpp, src/app.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 17,2 % (56 340 B), Flash 72,1 % (1 417 923 B). `pio test -e native -f test_ascii` → 2 Tests, 0 Fehler
- Gegenprobe: `ü → "u"` statt `"ue"` → `test_umlauts` und `test_truncates` FAILED; zurück → 2 PASS
- Nächster Schritt: Task 11 Step 1
- Offen/Fragen: Abweichungen: `menu_touch(x, y, now_ms)` und `settings_touch(x, y, now_ms)` bekommen die Zeit (für den 10-s-Timeout). Zusätzlich `settings_tick`, `settings_timed_out` (30 s, nicht im Plan) und `settings_firmware_source`. Langes Drücken öffnet das Menü nicht, solange der Buddy schläft (dunkles Display, sichere Variante).
- Am Gerät zu prüfen: Kachel-Treffer (Touch-Kalibrierung), Lesbarkeit der Uhr, Helligkeitsstufen.

## Task 9 – Netz (net.*) – ERLEDIGT
- Zeit: 2026-10-25 10:35 UTC
- Gemacht: WiFiManager nicht blockierend (AP „CYD-Buddy“, Feld „Ort“ id `place`, 47 Zeichen), Portal beim ersten Start bzw. nach 30 s ohne WLAN; NTP mit `configTzTime`; Geocoding per HTTPS (`NetworkClientSecure::useBuiltinCACertBundle()` – im Core-Header `NetworkClientSecure.h` nachgesehen; Bundle ist laut sdkconfig `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL`); Wetter alle 30 min per HTTP, Fehler → 5 min; JSON mit Filter. Abrufe im `net_task` (Kern 0, Prio 1, Stack 8192); nur der Haupt-Task schreibt `Persist`/NVS. `app.cpp`: Stunde an mood, Statusstreifen aus net, Wetter-Overlays (nur frisch < 3 h), Aus-Zeit einmalig nachgerechnet, Epoche beim Speichern.
- Dateien: src/net/net.h, src/net/net.cpp, src/app.cpp
- Prüfung: Step 1 `curl` → **nicht geprüft, weil** das Cloud-Netz `geocoding-api.open-meteo.com` sperrt (403 vom Proxy) und `api.open-meteo.com` nicht auflöst. `pio run -e cyd2usb` → SUCCESS, RAM 17,2 % (56 292 B), Flash 71,6 % (1 407 107 B von 1 966 080 B)
- Gegenprobe: –
- Nächster Schritt: Task 10 Step 1
- Offen/Fragen: (1) URLs ungeprüft – lokal mit den beiden `curl`-Befehlen aus Plan Task 9 Step 1 gegenprüfen (Wetter bleibt wie in der Spec auf `http://`). (2) Sicherheit: WiFiManager registriert die Upload-Seite `/update` immer, auch wenn sie im Menü fehlt; das Portal-AP ist offen. Deshalb Menü nur „wifi, exit“, Info-Update aus und `setPreOtaUpdateCallback` → `ESP.restart()`, damit niemand über das Portal Firmware aufspielen kann. (3) Von Hand geöffnetes Portal schließt nach 5 min wieder, wenn WLAN verbunden ist (nicht im Plan, damit das AP nicht ewig offen bleibt). (4) Der Ort wird nach dem Geocoding durch den Namen aus der Antwort ersetzt (z. B. „Berlin“).
- Am Gerät zu prüfen: Portal „CYD-Buddy“ erscheint, Ort-Feld, Verbindung, Uhrzeit, Wetter im Statusstreifen.

## Task 8 – Augen zeichnen und Hauptschleife – ERLEDIGT
- Zeit: 2026-10-25 10:31 UTC
- Gemacht: Augen-Renderer mit 8-Bit-Sprite 320×200 (Fallback direkt), Overlays (Herz, Sonnenbrille, Schweiß, Träne, zZz, Zittern, Regen-Tönung), Symbole aus Primitiven, Statusstreifen (zeichnet nur bei Änderung), `app.cpp` mit Bildtakt 33 ms, Gesten → Stimmung, Licht → Schlaf, Blinzeln/Blick/Tween, LED 15 %, Hintergrundlicht, Speichern alle 10 min, Seriell-Log alle 60 s. Testbild nach `ui/testscreen.cpp` ausgelagert (beim Start gedrückt halten).
- Dateien: src/ui/eyes.h, src/ui/eyes.cpp, src/ui/icons.h, src/ui/icons.cpp, src/ui/status_bar.h, src/ui/status_bar.cpp, src/ui/testscreen.h, src/ui/testscreen.cpp, src/app.h, src/app.cpp, src/main.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 10,6 % (34 736 B statisch; Sprite 64 000 B kommt zur Laufzeit dazu), Flash 27,3 % (537 303 B). `pio test -e native` → 45 Tests, 0 Fehler (7 Gruppen)
- Gegenprobe: – (Renderer nicht unit-getestet)
- Nächster Schritt: Task 9 Step 1
- Offen/Fragen: (1) Symbol-Funktionen nehmen `LovyanGFX &` statt `LGFX &` (Basisklasse; `LGFX` passt weiter, dazu gehen Sprites). Zusätzliche Symbole: `icon_target`, `icon_simon`, `icon_brightness`, `icon_back`. Zusätzlich `status_invalidate()` und `eyes_draw_pair()` (für Spiele/OTA). (2) Lid-Schnitte beziehen sich auf die *geöffnete* Höhe `h*open`, nicht auf `h` – sonst wäre SLEEPY (open 0,35, top_cut 0,5) ganz zu. (3) Sonnenbrillen-Rahmen grau statt schwarz, weil Schwarz auf schwarzem Grund unsichtbar wäre. (4) Mond-Symbol setzt schwarzen Hintergrund voraus.
- Am Gerät zu prüfen: Augen flüssig (fps im Seriell-Log, Ziel ≥ 25), Gesten (Tippen/Streicheln/Stupsen/Lang drücken), LDR-Richtung/-Schwelle, Testbild beim Start gedrückt halten.

## Task 7 – Hardware-Schicht (LED, Licht, Speicher) – ERLEDIGT
- Zeit: 2026-10-25 10:27 UTC
- Gemacht: `led.*` (ledcAttach/ledcWrite, im Core-Header `esp32-hal-ledc.h` geprüft: `bool ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution)`, `bool ledcWrite(uint8_t pin, uint32_t duty)`), `light.*` (Ringpuffer 16, alle 60 ms, Hysterese), `store.*` (Preferences, Namensraum `buddy`, Versions-Schlüssel `ver`).
- Dateien: src/hw/led.h, src/hw/led.cpp, src/hw/light.h, src/hw/light.cpp, src/hw/store.h, src/hw/store.cpp
- Prüfung: `pio run -e cyd2usb` → SUCCESS, RAM 10,4 % (33 984 B), Flash 25,2 % (494 623 B) – Module noch nicht benutzt
- Gegenprobe: – (keine Unit-Tests für Hardware vorgesehen)
- Nächster Schritt: Task 8 Step 1
- Offen/Fragen: `LDR_DARK_IS_HIGH` fehlt in config.h → in light.cpp Standard 1 mit Kommentar „am Gerät messen“. Sichere Variante: Solange `LDR_DARK_THRESHOLD` ≤ 0 (Platzhalter), meldet `light_update` nie „dunkel“, sonst würde der Buddy mit Schwelle 0 dauernd schlafen.

## Task 6 – OTA-Regeln portieren (ota_policy.h) – ERLEDIGT
- Zeit: 2026-10-25 10:26 UTC
- Gemacht: Vorlage kopiert, `OTA_CHIP_ESP32 = 0`, Kommentar auf `test/test_ota_policy`, neu `ota_redirect_ok(url)` (nur `https://`, Host `github.com` oder Endung `.githubusercontent.com`, Groß/Klein egal, Host nur aus Buchstaben/Ziffern/`-`/`.`).
- Dateien: src/logic/ota_policy.h, test/test_ota_policy/test_main.cpp
- Prüfung: `pio test -e native -f test_ota_policy` vor Implementierung → ERRORED (Header fehlt); mit unveränderter Vorlage → SIGSEGV; nach Korrektur → 5 Tests, 0 Fehler
- Gegenprobe: Chip-Konstante 0 → 9 → `test_image_ok_esp32: Expected 0 Was 3` und `test_image_rejects` FAILED; zurück → 5 PASS
- Nächster Schritt: Task 7 Step 1
- Offen/Fragen: **Abweichung von der Vorlage:** `OTA_IMG_CHECK_BYTES` war 176, `ota_image_check` liest die ELF-SHA aber aus Byte 176…207 (`32 + 144 + i`). Mit genau 176 Byte Puffer (wie im Test) liest es über das Ende hinaus → Absturz. Jetzt `32 + 176 = 208`. Dasselbe steckt in der Vorlage `ota-publish.mjs` (`CHECK_BYTES = 176`, liest bis 208) und betrifft vermutlich auch den HA-Buddy (dort harmlos, weil der Puffer größer ist). In Task 13 im Skript ebenfalls auf 208 gesetzt.

## Task 5 – Spiel-Logik (simon.h, catch_game.h) – ERLEDIGT
- Zeit: 2026-10-25 10:24 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/simon.h` und `src/logic/catch_game.h` nach den Interfaces im Plan (Zufall über `rng_range` aus eye_model.h).
- Dateien: src/logic/simon.h, src/logic/catch_game.h, test/test_games/test_main.cpp
- Prüfung: `pio test -e native -f test_games` vor Implementierung → ERRORED (Header fehlt); danach → 7 Tests, 0 Fehler
- Gegenprobe: Gewinnprüfung in `simon_press` auf `len == 5` → `test_simon_win: Expected 0 Was 2 [FAILED]`; zurück → 7 PASS
- Nächster Schritt: Task 6 Step 1
- Offen/Fragen: –

## Task 4 – Wetter-Laune und Overlays (weather_mood.h) – ERLEDIGT
- Zeit: 2026-10-25 10:24 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/weather_mood.h` mit WMO-Zuordnung, Flags hot/cold, `weather_fresh` (< 3 h), `overlays_for`.
- Dateien: src/logic/weather_mood.h, test/test_weather_mood/test_main.cpp
- Prüfung: `pio test -e native -f test_weather_mood` vor Implementierung → ERRORED (Header fehlt); danach → 4 Tests, 0 Fehler
- Gegenprobe: `> 28` → `>= 28` → `test_flags: Expected FALSE Was TRUE [FAILED]`; zurück → 4 PASS
- Nächster Schritt: Task 5 Step 1
- Offen/Fragen: –

## Task 3 – Augenmodell (eye_model.h) – ERLEDIGT
- Zeit: 2026-10-25 10:23 UTC
- Gemacht: Test aus dem Plan wörtlich; `src/logic/eye_model.h` mit Tabelle (switch), xorshift32, `eye_tween`/`rgb_tween` (k = 1 − e^(−dt/150)), Blinker und Gazer nach Plan.
- Dateien: src/logic/eye_model.h, test/test_eye_model/test_main.cpp
- Prüfung: `pio test -e native -f test_eye_model` vor Implementierung → ERRORED (Header fehlt); danach → 6 Tests, 0 Fehler
- Gegenprobe: Blinzelabstand 2000–6000 → 7000–9000 → `test_blinker_closes_within_window` und `test_blinker_reopens` FAILED; zurück → 6 PASS
- Nächster Schritt: Task 4 Step 1
- Offen/Fragen: –

## Task 2 – Gesten (gesture.h) – ERLEDIGT
- Zeit: 2026-10-25 10:23 UTC
- Gemacht: Test aus dem Plan wörtlich, `src/logic/gesture.h` nach den Regeln im Plan (Tipp-Ringpuffer der letzten 5, Streichel-Fenster 1,5 s, Bewegungsschwelle 15 px).
- Dateien: src/logic/gesture.h, test/test_gesture/test_main.cpp
- Prüfung: `pio test -e native -f test_gesture` vor Implementierung → ERRORED (Header fehlt); danach → 7 Tests, 0 Fehler
- Gegenprobe: `GESTURE_STROKE_PX` 60 → 200 → `test_stroke: Expected 2 Was 0 [FAILED]`; zurück → 7 PASS
- Nächster Schritt: Task 3 Step 1
- Offen/Fragen: –

## Task 1 – Stimmung (mood.h) – ERLEDIGT
- Zeit: 2026-10-25 10:22 UTC
- Gemacht: Test aus dem Plan wörtlich übernommen, `src/logic/mood.h` exakt nach Plan.
- Dateien: src/logic/mood.h, test/test_mood/test_main.cpp
- Prüfung: `pio test -e native -f test_mood` vor Implementierung → ERRORED (`logic/mood.h: No such file`); danach → 13 Tests, 0 Fehler
- Gegenprobe: `+= 8` → `+= 7` in STROKE → `test_stroke_cooldown: Expected 58 Was 57 [FAILED]`; zurückgeändert → 13 PASS
- Nächster Schritt: Task 2 Step 1
- Offen/Fragen: –

## Task 0 – Werkzeuge (Cloud) – ERLEDIGT mit Umweg
- Zeit: 2026-10-25 10:22 UTC
- Gemacht: PlatformIO 6.2.0 per pip. Die PlatformIO-Registry (`api.registry.platformio.org`, `collector.platformio.org`) ist im Cloud-Netz gesperrt (403), GitHub geht. Umweg nur außerhalb des Repos: Proxy-CA an `~/.platformio/penv/.../certifi/cacert.pem` angehängt (pioarduino überschreibt `REQUESTS_CA_BUNDLE`), `tool-scons` 4.11.1 aus PyPI nach `~/.platformio/packages/tool-scons` gelegt, `platform-native` aus GitHub nach `~/.platformio/platforms/native`, Bibliotheken per `git clone` mit `.piopm` nach `.pio/libdeps/` (LovyanGFX 1.2.34, WiFiManager 2.0.17, ArduinoJson 7.4.3, Unity 2.6.1; bei Unity Kommentarzeilen aus `library.json` entfernt). `platformio.ini` unverändert.
- Dateien: keine im Repo
- Prüfung: `pio test -e native -f test_smoke` → 3 Tests, 0 Fehler. `pio run -e cyd2usb` → SUCCESS, RAM 10,4 % (33 984 B), Flash 25,2 % (494 623 B von 1 966 080 B)
- Gegenprobe: –
- Nächster Schritt: Task 1 Step 1
- Offen/Fragen: Lokal mit normalem Internet ist kein Umweg nötig. Branch: Die Cloud-Sitzung darf nur auf `claude/dreamy-galileo-bvdw39` pushen, nicht auf `buddy-umsetzung` (Vorgabe der Sitzung). Der lokale Claude übernimmt also `claude/dreamy-galileo-bvdw39` (ggf. lokal in `buddy-umsetzung` umbenennen).

## Task 0 – Gerüst und Testbild – ERLEDIGT (lokal)
- Zeit: 2026-10-25
- Gemacht: PlatformIO-Gerüst (cyd2usb + native), Pins/Display-Klasse, Testbild, Smoke-Test, OTA-Vorlagen aus dem HA-Buddy unter reference/.
- Dateien: platformio.ini, src/config.h, src/hw/display.h, src/main.cpp, src/logic/clamp.h, test/test_smoke/, reference/ha_buddy_ota/
- Prüfung: siehe Commit „chore: Gerüst“ (Build und Smoke-Test lokal)
- Gegenprobe: Smoke-Test mit falschem Erwartungswert schlägt fehl
- Nächster Schritt: Task 1 Step 1
- Offen/Fragen: Panel-Variante, Touch-Kalibrierung, LDR-Richtung/-Schwelle misst der lokale Claude am Gerät.
