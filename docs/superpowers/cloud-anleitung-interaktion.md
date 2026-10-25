# Anleitung für den Online-Agenten (claude.ai/code): CYD-Buddy – Interaktion F1–F4

Du setzt den Plan `docs/superpowers/plans/2026-10-25-interaktion.md` um, Task 1 bis 10, in dieser Reihenfolge. Lies vorher **vollständig**:
- den Plan
- die Spec `docs/superpowers/specs/2026-10-25-interaktion-design.md`
- die Gesichts-Spec `docs/superpowers/specs/2026-10-25-gesicht-design.md`
- `docs/superpowers/specs/2026-10-25-saetze.json`
- die obersten Einträge von `docs/FORTSCHRITT.md`

Danach liest du `src/app.cpp`, `src/logic/mood.h`, `src/logic/face_model.h`, `src/ui/eyes.cpp`, `src/ui/menu.*` und `src/hw/store.*`.

## Das Wichtigste: Jeder Schritt wird einzeln dokumentiert
Deine Token können jederzeit ausgehen. Dann übernimmt der lokale Claude, und er sieht **nur, was gepusht ist**. Deshalb:
1. **Nach jedem Task** schreibst du einen Eintrag oben in `docs/FORTSCHRITT.md` (Format im Plan), dann commit und push. Erst danach kommt der nächste Task.
2. **Bei UI-Tasks (2, 5, 7, 9)** zusätzlich nach jedem Step, der Dateien geändert hat: Eintrag mit Status `TEILWEISE` und genau dem nächsten Step, dann commit und push. Halbfertiger Code darf committet werden, wenn FORTSCHRITT sagt, was fehlt.
3. Prüfergebnisse trägst du wörtlich ein. „Nicht geprüft, weil …“ ist erlaubt, Weglassen nicht.
4. Abweichungen vom Plan oder der Spec kommen mit Begründung unter „Offen/Fragen“.
5. Die Checkboxen im Plan hakst du nicht ab.

## Ablauf
1. Basis ist der Branch **`buddy-umsetzung`** (`git fetch origin && git checkout buddy-umsetzung && git pull`). Darfst du darauf nicht pushen, legst du davon deinen eigenen Branch an und vermerkst seinen Namen im ersten FORTSCHRITT-Eintrag.
2. Werkzeuge: `pip install platformio`, dann `pio test -e native` (78 Tests grün) und `pio run -e cyd2usb`. Beim letzten Cloud-Lauf war die PlatformIO-Registry gesperrt. Wie das umgangen wurde (Pakete von GitHub und PyPI, außerhalb des Repos), steht in FORTSCHRITT unter „Task 0 – Werkzeuge“. Mach es genauso. Geht etwas nicht, trägst du `BLOCKIERT` mit der Fehlermeldung ein und machst trotzdem weiter: Logik und Tests gehen auch ohne Gerätebuild.
3. Danach kommen Task 1 bis 10 nach Plan, mit TDD, Gegenprobe und je einem Commit. Trailer: `Co-Authored-By: Claude <dein Modell> <noreply@anthropic.com>`.

## Was du NICHT tust
- Nicht auf `main` pushen, kein PR, kein Merge.
- Kein `ota-publish --release`, kein `gh release`, nicht flashen. Das macht der lokale Claude.
- `src/ota/*`, `src/logic/ota_policy.h`, `tools/ota-publish.mjs`, `src/hw/display.h`, `src/config.h` und die Touch-Kalibrierung in `src/main.cpp` lässt du unverändert.
- Keine neuen Hardware-Annahmen (kein Lautsprecher, keine Sensoren).
- Keine Zugangsdaten und kein `setInsecure()`.
- Die Sätze in `saetze.json` änderst du nicht. Fehlt dir ein Satz, notierst du ihn unter „Offen/Fragen“.

## Hilfen
- API-Namen von LovyanGFX stehen in `.pio/libdeps/cyd2usb/LovyanGFX/src/`, die des Arduino-Core in `~/.platformio/packages/framework-arduinoespressif32*/`. Nachsehen, nicht aus dem Gedächtnis schreiben.
- Die Schriften haben keine Umlaute. Text immer über `logic/ascii.h` wandeln, wie es die bestehenden UI-Module tun.
- Bildschirme und Menü: Vorbild ist der vorhandene Code (`ui_tile`, `ui_hit`, `ui_text` in `src/ui/ui_common.*`).

## Wenn du fertig bist oder aufhören musst
Letzter FORTSCHRITT-Eintrag mit Status, nächstem Schritt und der Liste „am Gerät zu prüfen“, dann commit und push. Dem Nutzer schreibst du in einem Satz, bis zu welchem Task du gekommen bist und auf welchem Branch.
