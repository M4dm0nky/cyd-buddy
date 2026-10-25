# Firmware-Update per Abruf (7"-Display)

Das Gerät holt sich neue Firmware selbst. Ein Mensch am Display stimmt zu, danach läuft alles ohne Kabel.
Stand 2026-10-23 abends: gebaut (1 804 658 B, 57 %) und per Kabel geflasht. Als erstes echtes Update ist der Design-Build (1 808 112 B) veröffentlicht, **am Display noch nicht bestätigt**.
**Vor dem Übernehmen von Cloud-Branches** mit `git cherry -v HEAD origin/<branch>` prüfen, was fehlt; der Cloud-Branch enthält den alten OTA-Commit ohne die lokalen Korrekturen (Projektname, `.png`-Namen), ein Merge brächte den Projektnamen-Fehler zurück.
Das erste Mal muss die Firmware mit diesem Feature per Kabel aufgespielt werden (`tools/flash7.sh`); danach geht es ohne Kabel. Jede Änderung der Abruf-Adresse oder der Dateinamen kostet wieder einen Kabel-Flash.

## Ablauf

```
PC (lokaler Agent baut)                 Ablage                          Display
arduino-cli compile --output-dir ...    HA-Medienordner auf dem Pi
node tools/ota-publish.mjs --ha ──────►  /media/ha_buddy/          ◄──────  fragt alle 10 min manifest.png
  prüft Image, SHA-256, schreibt          manifest.png                    neue ELF-SHA-256? -> "Update bereit"
  firmware.png, dann manifest.png         firmware.png                    Tipp "Jetzt aktualisieren" (Zustimmung)
                                                                          Download -> SHA-256 -> inaktiver Slot
                                                                          Neustart, Gesundheitsnachweis
```

## Zustimmung und Sicherheit

1. **Zustimmung vor Ort.** Heruntergeladen wird erst nach einem Tipp auf *Jetzt aktualisieren*. *Später* und jeder Tipp daneben schlummern eine Stunde.
   Die Karte erscheint nur bei wachem Display (nicht gedimmt/dunkel, damit ein Tipp in ein dunkles Display nie zustimmt) und ignoriert Tipps in der ersten Sekunde.
2. **Anmeldung.** Das Gerät schickt `Authorization: Bearer <Token>`. Standard: der HA-Token gegen HAs Medienordner (`/media/local/ha_buddy/`, nur mit Anmeldung; ohne Token 401, gemessen).
   **Die `.bin` enthält WLAN-Zugang und HA-Token** (kompiliert aus `secrets.h`). Sie darf nie unter einer Adresse ohne Anmeldung liegen: nicht in HAs `/local/` (= `www`), nicht in einem GitHub-Release, nicht auf einem offenen Webserver.
3. **Prüfung am Image selbst** (`src/logic/ota_policy.h`, nicht nach den Angaben im Manifest): Magic `0xE9`, Chip ESP32-S3, App-Kennung, Projektname gleich dem der **laufenden** Firmware (`esp_app_get_description()->project_name`; beim Waveshare-Core ist das `arduino-lib-builder`, nicht der Sketchname), ELF-SHA-256 gleich der im Manifest.
   Dazu SHA-256 über die ganze Datei, erst danach wird die Partition freigegeben (`Update.end`); bei Abweichung `Update.abort()`.
   Ein kaputtes oder fremdes Manifest meldet nie "Update bereit".
4. **A/B-Slot.** Das Image landet im inaktiven Slot (`Update`-Klasse des Arduino-Core). Bootloader, Partitionstabelle, NVS und der laufende Slot werden nicht beschrieben. Rettung bleibt USB (BOOT halten, einschalten, `tools/flash7.sh`).
5. **Rücksetzen bei kaputtem Image: nicht belegt.** `ota_pull` ruft nach dem Start `esp_ota_mark_app_valid_cancel_rollback()` erst, wenn UI-Takt läuft, HA verbunden ist und 20 s vergangen sind. Ob der Bootloader dieses Boards Rollback kann
   (`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE`), ist nicht geprüft; die Boot-Diagnose zeigt `OTA: <Partition> Zustand <n>` (Zustand 1 = PENDING_VERIFY heißt: Rollback aktiv). Ohne Rollback rettet nur der Kabel-Flash ein Image, das zwar gültig ist, aber nicht startet.
   **Gemessen (2026-10-23): kein Rollback.** Nach dem Kabel-Flash `OTA: app0 Zustand -1`; nach zwei Abruf-Updates `ota_slot` app1 bzw. app0 mit `ota_state` 2 (VALID) statt 1 (PENDING_VERIFY): der Bootloader setzt ein nicht bestätigtes Image nicht zurück. Ein gültiges, aber nicht startendes Image rettet nur der Kabel-Flash. Frühere Zeile dazu:  `OTA: app0 Zustand -1` (undefiniert). Das ist nach einem Kabel-Flash immer so und sagt nichts über Rollback; aussagekräftig ist erst der Zustand auf `app1` nach dem ersten Abruf-Update (1 = Rollback aktiv, 2/-1 = kein Rollback). Eintragen, sobald gesehen. Seit der Telemetrie-Erweiterung (Compile pending) steht der Wert in HA: `sensor.ha_buddy_7in_diag` Attribute `ota_slot` (app0/app1), `ota_state` (Zustand beim Start) und `elf12`/`build` (welche Firmware läuft, vergleichbar mit dem Manifest).

## Dateien

| Datei | Aufgabe |
|---|---|
| `HA_Buddy_7in/src/ota/ota_pull.*` | Manifest holen (Task `ha_http`, alle 10 min, 3 min nach Fehler), Download, SHA-256, `Update`, Neustart, Gesundheitsnachweis |
| `HA_Buddy_7in/src/ui/ota_notice.*` | Karte auf `lv_layer_top()`: Update bereit / Fortschritt / Neustart / Fehler |
| `HA_Buddy_7in/src/logic/ota_policy.h` | Image-Prüfung, Vergleich, Schlummern, Fortschritt (Host-Test in `selftest.cpp`) |
| Status-Seite, Zeile "Firmware" (`ui/scr_status.cpp`, `ota_status_line` in `ota_policy.h`) | Tippen: `ota_pull::check_now()`; Anzeige Prüfe …/Aktuell (vor x min)/Update bereit/Fehler; Suche im eigenen Task `ha_ota` (nicht hinter `ha_rest`/`ha_energy`) |
| `tools/ota-publish.mjs` | Prüft die gebaute `.bin`, schreibt `firmware.png` + `manifest.png` (Manifest zuletzt, atomar); `--ha` lädt beide in HAs Medienordner hoch; `--serve` bietet den Ordner mit Token an (PC-Server, vom Nutzer nicht gewollt) |

## Einstellungen (`secrets.h`, beide optional)

```c
#define SECRET_OTA_BASE_URL "http://<PC-IP>:8765"   // Ordner mit manifest.json + .bin, ohne Schrägstrich am Ende
#define SECRET_OTA_TOKEN    "64-hex-zeichen"             // Bearer für diesen Server
```
Ohne beides: `http://<HA_HOST>:<HA_PORT>/media/local/ha_buddy/` mit dem HA-Token (Dateien `manifest.png`, `firmware.png`). Ein Token zu wechseln kostet einen Kabel-Flash (das Gerät authentifiziert sich mit dem einkompilierten).

## Alltag

```
arduino-cli compile --fqbn <FQBN aus HA_Buddy_7in/README.md> --output-dir HA_Buddy_7in/build HA_Buddy_7in
node tools/ota-publish.mjs --dest ota-out --ha      # prüft, schreibt nach ota-out/ und lädt nach HA /media/ha_buddy/ hoch (Zugang aus secrets.h)
```
Gates des Skripts: Image gültig und vom richtigen Projekt; Quellcode nicht neuer als die `.bin` (`--allow-stale`); keine uncommitteten Änderungen (`--allow-dirty`), damit nicht ein
Stand verschickt wird, den es im Repo nicht gibt. Am Display innerhalb von ca. 10 min *Update bereit* -> *Jetzt aktualisieren*.

## Ablage wählen

- **HA-Medienordner (Standard, gemessen 2026-10-23).** HA läuft auf einem Raspberry Pi (<HA-IP>), `/media` steht in `allowlist_external_dirs`. Zwei Einschränkungen von HA:
  (1) `POST /api/media_source/local_source/upload` nimmt nur Dateien mit Audio-/Video-/Bild-Content-Type an (sonst 400), legt `ha_buddy/` aber selbst an;
  (2) `GET /media/local/...` liefert nur Dateien mit Audio-/Video-/Bild-**Endung** aus (`.bin`/`.json` -> 404).
  Darum heißen die Dateien `firmware.png` und `manifest.png` (Inhalt unverändert, Upload mit `type=image/png`). Abruf nur mit Bearer-Token (ohne 401, mit Token 200 und gleiche SHA-256). Prüfen:
  `curl -H "Authorization: Bearer <HA-Token>" http://<HA>:8123/media/local/ha_buddy/manifest.png`. Kein SSH/Samba-Zugang zum Pi nötig. Löschen alter Dateien geht über die HA-Oberfläche oder die WebSocket-API (`media_source/local_source/remove`).
- **PC-Server.** `node tools/ota-publish.mjs --serve 8765` (Token aus `SECRET_OTA_TOKEN` in `secrets.h`, `--token` oder `OTA_TOKEN`). Nur erreichbar, solange der PC läuft und die Windows-Firewall Port 8765 im LAN erlaubt.

## Grenzen / offen

- Nicht gebaut: Funktionsnamen gegen echte Header geprüft (`Update.h` 3.1.1, `esp_ota_ops.h`/`esp_app_desc.h` IDF 5.3.2), Linken und Verhalten offen.
- Partitionstabelle `app3M_fat9M_16MB` muss zwei App-Slots haben (der Core-Standard hat sie); `Update.begin` meldet sonst `Update.begin: ...` als Fehler auf dem Display.
- Download blockiert den `ha_http`-Task (bis ~30 s): in der Zeit keine REST-Abfragen. Das Display bleibt bedienbar.
- Zeit/Speicher: ein 4-KB-Puffer im PSRAM, kein großes JSON (Manifest ~300 B). `json_errors` und PSRAM-Minimum nach einem Update im Auge behalten.
