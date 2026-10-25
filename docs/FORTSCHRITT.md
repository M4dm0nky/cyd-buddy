# Fortschritt CYD-Buddy

Neueste Einträge oben. Format und Regeln: `docs/superpowers/plans/2026-10-25-cyd-buddy.md`, Abschnitt „Fortschritts-Protokoll“.

## Task 0 – Gerüst und Testbild – ERLEDIGT (lokal)
- Zeit: 2026-10-25
- Gemacht: PlatformIO-Gerüst (cyd2usb + native), Pins/Display-Klasse, Testbild, Smoke-Test, OTA-Vorlagen aus dem HA-Buddy unter reference/.
- Dateien: platformio.ini, src/config.h, src/hw/display.h, src/main.cpp, src/logic/clamp.h, test/test_smoke/, reference/ha_buddy_ota/
- Prüfung: siehe Commit „chore: Gerüst“ (Build und Smoke-Test lokal)
- Gegenprobe: Smoke-Test mit falschem Erwartungswert schlägt fehl
- Nächster Schritt: Task 1 Step 1
- Offen/Fragen: Panel-Variante, Touch-Kalibrierung, LDR-Richtung/-Schwelle misst der lokale Claude am Gerät.
