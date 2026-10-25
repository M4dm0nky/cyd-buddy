# Anleitung für den Online-Agenten (claude.ai/code): CYD-Buddy umsetzen

Du setzt den Plan `docs/superpowers/plans/2026-10-25-cyd-buddy.md` um, Task 1 bis 14, in dieser Reihenfolge.
Die Regeln stehen in der Spec `docs/superpowers/specs/2026-10-25-cyd-buddy-design.md`.
Lies beide Dateien vollständig, bevor du anfängst.

## Das Wichtigste: Jeder Schritt wird einzeln dokumentiert

Deine Token können jederzeit ausgehen. Dann macht der lokale Claude an genau der Stelle weiter, an der du aufgehört hast. Er sieht dafür **nur, was gepusht ist**. Deshalb gilt:

1. **Nach jedem Task** schreibst du einen Eintrag **oben** in `docs/FORTSCHRITT.md`. Das Format steht im Plan unter „Fortschritts-Protokoll“. Danach `git commit` und `git push`. Erst dann beginnst du den nächsten Task.
2. **Ein Task dauert länger als ein paar Steps** (vor allem Task 8 bis 12)? Dann zusätzlich nach jedem Step, der Dateien geändert hat: Eintrag mit Status `TEILWEISE` und „Nächster Schritt: Task N Step M“, dann commit und push. Halb fertiger Code darf committet werden, solange FORTSCHRITT genau sagt, was fehlt.
3. **Prüfergebnisse wörtlich** eintragen: Befehl, Anzahl Tests und Fehler, Build-Größe. Wenn du etwas nicht prüfen konntest, schreib „nicht geprüft, weil …“. Lass so etwas nie stillschweigend weg.
4. **Abweichungen vom Plan** (andere Funktionsnamen im Core, andere Bibliotheksversion, URL antwortet anders) kommen unter „Offen/Fragen“, mit Begründung.
5. Die Checkboxen im Plan hakst du **nicht** ab. Der Stand steht nur in FORTSCHRITT. So gibt es keine Konflikte in der Plan-Datei.

## Ablauf

1. Branch: `git checkout -b buddy-umsetzung`. Existiert er schon auf origin, nimm `git checkout buddy-umsetzung && git pull`.
2. Lies `docs/FORTSCHRITT.md`. Steht dort schon ein Eintrag von dir oder einem früheren Lauf, mach beim dort genannten „Nächster Schritt“ weiter.
3. Werkzeuge einrichten:
   ```bash
   pip install platformio
   pio --version
   pio test -e native -f test_smoke     # muss 3 Tests PASS zeigen
   pio run -e cyd2usb                   # erster Lauf lädt die Plattform (mehrere Minuten)
   ```
   Klappt etwas davon nicht (kein Internet für pip/PlatformIO, kein gcc): Eintrag „Task 0 – Werkzeuge – BLOCKIERT“ mit der Fehlermeldung, commit, push. Arbeite dann trotzdem weiter und vermerke bei jedem Task „nicht gebaut/getestet, weil …“.
4. Danach Task 1 bis 14 nach Plan. Pro Task genau ein Commit (plus Zwischen-Commits nach Punkt 2 oben). Commit-Nachricht wie im Plan angegeben, mit dem Trailer
   `Co-Authored-By: Claude <dein Modell> <noreply@anthropic.com>`.
5. Am Ende Task 14 mit der Abschlussprüfung. Dann **keinen** PR öffnen, nur pushen.

## Was du NICHT tust

- Nicht auf `main` pushen, kein PR, kein Merge.
- **Kein** `node tools/ota-publish.mjs --release` und kein `gh release`. Veröffentlichen macht nur der lokale Claude.
- `src/config.h` und `src/hw/display.h` nur ändern, wenn ein Task das ausdrücklich verlangt. Pins, Panel-Variante und Kalibrierung misst der lokale Claude am Gerät.
- Keine Annahmen über die Hardware, die über Plan und Spec hinausgehen. Im Zweifel unter „Offen/Fragen“ notieren und mit der sicheren Variante weitermachen.
- Kein `setInsecure()`, keine Tokens oder Passwörter im Code.
- Dateien unter `reference/` nicht ändern. Das sind Vorlagen.

## Hilfen

- Die Vorlagen für OTA aus dem HA-Buddy liegen in `reference/ha_buddy_ota/`. Kopieren und anpassen, nicht kompilieren.
- Die Funktionsnamen im Arduino-Core 3.x (Zertifikatspaket, `NetworkClientSecure` bzw. `WiFiClientSecure`, `ledcAttach`) prüfst du in den Headern unter `~/.platformio/packages/framework-arduinoespressif32*/`. Schreib sie nicht aus dem Gedächtnis.
- Die LovyanGFX-API steht in `.pio/libdeps/cyd2usb/LovyanGFX/src/`.
- Die Open-Meteo-URLs vor dem Einbauen mit `curl` prüfen (Task 9, Step 1).

## Wenn du fertig bist oder aufhören musst

Letzter Eintrag in FORTSCHRITT: Status, „Nächster Schritt“ und die Liste „am Gerät zu prüfen“. Dann commit und push.
Dem Nutzer schreibst du in einem Satz, bis zu welchem Task du gekommen bist und dass der lokale Claude den Branch `buddy-umsetzung` übernehmen kann.
