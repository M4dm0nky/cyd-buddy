# Fortschritt CYD-Buddy

Neueste Einträge oben. Format und Regeln: `docs/superpowers/plans/2026-10-25-cyd-buddy.md`, Abschnitt „Fortschritts-Protokoll“.

## Task 0 – Werkzeuge (Cloud) – ERLEDIGT mit Umweg
- Zeit: 2026-10-25 10:25 UTC
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
