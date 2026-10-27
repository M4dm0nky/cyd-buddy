# Roboter (Stufe 3): Druckteile zum Zusammenstecken, mehrfarbig

Stand 2026-10-27. Baut auf `2026-10-26-roboter-gehaeuse-design.md` auf (Form, Strom, Farben) und macht aus dem Konzept druckbare Teile.

## Ziel
- Jedes Teil wird in **einer** Farbe gedruckt. Die Teile werden gesteckt und, wo es nötig ist, mit M3 verschraubt.
- Zusätzlich gibt es eine 3MF für Mehrfarbdrucker (AMS). Darin liegen alle Teile als Körper, die Zierschrauben sind als Einlage eingesetzt.
- PETG, ohne Stützen, Wandstärke ≥ 2 mm, Passungsspiel `fit`.

## Dateien
| Datei | Inhalt |
|---|---|
| `case/cyd_common.scad` | **neu.** Maße von CYD, Display und Fenster, Lochbild, `window_cut()`, Prüfkörper für Platine, Glas und aktive Fläche. Keine Ausgabe auf oberster Ebene, wird per `include` eingebunden. |
| `case/madbuddy_case.scad` | Stufe 2. Nutzt `cyd_common.scad`, **die Geometrie bleibt bitgenau gleich.** |
| `case/madbuddy_roboter.scad` | **neu.** Alle Roboterteile, Auswahl per `part=`. |
| `case/check_roboter.ps1` | **neu.** Prüfung, aufgebaut wie `check.ps1`. |
| `case/tools/stl2_3mf.py` | Übernommen aus `HA_BUDDY/gehaeuse/tools/`. Erweitert um eine Option, die mehrere STL-Dateien als Teile **eines** Objekts schreibt (für AMS). |
| `case/stl/roboter_*.stl`, `case/3mf/roboter_*.3mf`, `case/render/roboter_*.png` | Ausgabe |

## Teile
| Teil (`part=`) | Farbe | Inhalt und Verbindung |
|---|---|---|
| `head_front` | blau | Kopfschale mit Display-Fenster und CYD-Halterung wie in Stufe 2 (gleiche Parameter). 4 Schraubdome M3. Flache Mulde für die Blende. Je ein Steckloch für die Ohren. |
| `bezel` | schwarz | Gesichtsblende, flacher Rahmen um das Fenster (≥ 1,2 mm). Sie wird in die Mulde gesteckt (Presspassung, bei Bedarf Kleber). Die Fensterkante bildet die Blende, die Fase gilt wie in Stufe 2. |
| `head_back` | blau | Rückwand mit Falz. Öffnung für USB-C des CYD, Fenster für BOOT/RESET. Unten die Aufnahme für den Hals. 4× M3 von hinten. |
| `ear` (2×) | gelb | Runde Scheibe Ø 30 mm mit Zapfen, wird seitlich in den Kopf gesteckt. Die Zierschraube in der Mitte ist erhaben. |
| `neck` | schwarz | Rohr Ø 22 mm mit Kabelkanal ≥ 10 mm. Oben und unten je ein Flansch mit 2× M3. Die Schrauben sind von innen erreichbar, wenn die Rückwände von Kopf bzw. Oberkörper abgenommen sind. |
| `torso_front` | orange | Brust mit Linsenloch, Kamera um 15° nach oben gekippt, Klemmrippen und Anschlag für den XIAO. Steckaufnahmen für die Schulterzapfen. Zierschrauben erhaben. |
| `torso_back` | orange | Rückwand mit Lüftungsschlitzen, M3. |
| `arm` (2×, gespiegelt) | orange | Arm mit Schulterkugel und Zapfen in den Oberkörper. Am Ende ein Zapfen für die Hand. Die Hand liegt auf dem Oberschenkel. |
| `hand` (2×) | schwarz | Wird auf den Arm gesteckt. |
| `seat` | blau | Po und Oberschenkel, oberer Teil. Darin das Akkufach (DFR0969 quer). Der Oberkörper wird aufgesteckt und von unten mit M3 verschraubt. Je ein Steckzapfen für die Füße. |
| `bottom` | blau | Boden. Aussparung für die USB-C-Ladebuchse, Loch für die Lade-LED, Schiebeschalter, Öffnung für den HOLD/NORMAL-Schalter. 4× M3 in `seat`. |
| `foot` (2×) | schwarz | Fuß, wird vorn auf das Bein gesteckt, Spitze nach oben. |

Form und Maße kommen aus `case/konzept/roboter_konzept.scad`:
- Kopf 114 × 84 × 32 mm, 8° nach hinten geneigt
- Oberkörper 64 × 44 × 48 mm
- Po etwa 112 × 58 × 32 mm
Wo der Inhalt mehr Platz braucht, gehen die Innenmaße vor. Die Außenmaße passen sich dann an.

**Steckverbindungen:**
- Zapfen rund mit Abflachung, damit sie nicht drehen. Mindestens Ø 6 mm und 6 mm tief.
- Spiel `fit_peg` als eigene Variable, Vorgabe 0,15 mm, als Presspassung gedacht.
- Fasen an den Zapfen erleichtern das Einführen.

## Akku (VORLÄUFIG nach Datenblatt)
- DFR0969: Maße der Platine, Lage der USB-C-Buchse, der Micro-USB-Buchse, des HOLD/NORMAL-Schalters, der 5-V-/GND-Stifte und der LED. Quelle sind Datenblatt und Produktfoto, jeder Wert ist im SCAD mit `VORLÄUFIG (Datenblatt)` markiert.
- Das Fach bekommt `batt_clear = 1,0 mm` Zusatzspiel.
- Zellen 18650: Ø 18,6 × 65 mm, mit Polkappen bis 69 mm.
- Schiebeschalter: VORLÄUFIG, typischer Mini-Schiebeschalter (SS12D00, etwa 12 × 6 × 6 mm, Hebel 4 mm). Wird von innen in eine Aussparung geklemmt.

## Kabel
- Akku-5V/GND → Schalter → durch die Öffnung von `seat` in den Oberkörper → Abzweig zum XIAO → durch den Hals → P1 am CYD.
- Die UART-Drähte gehen vom XIAO durch den Hals in den Kopf.
- Durchführungen ≥ 8 mm, damit ein JST-Stecker hindurchpasst.

## Druckausrichtung
- Jede STL liegt fertig ausgerichtet auf dem Bett und braucht keine Stützen.
- Überhänge höchstens 45°. Brücken nur kurz, höchstens 15 mm.
- Ist ein Teil so nicht druckbar, wird es geteilt.
- Die Kugel der Schulter wird unten abgeflacht.

## 3MF
- `roboter_teile.3mf`: alle Teile als **getrennte** Objekte. Im Slicer bekommt jedes Objekt seine Farbe. Teile gleicher Farbe kann man zusammen drucken.
- `roboter_ams.3mf`: Kopf vorn mit eingesetzter Blende und Zierschrauben als **ein** Objekt aus mehreren Teilen. Im Slicer ordnet man jedem Teil eine Farbe zu. Alle anderen Teile liegen als eigene Objekte dabei.

## Prüfung (`check_roboter.ps1`)
- STL-Export aller Teile ohne WARNING/ERROR, mit genug Facetten.
- Kollision: zusammengebaute Schalen gegen CYD (Platine, Glas, Rückseitenbauteile, Buchsen), aktive Fläche mit Sichtkegel, XIAO mit Linsenkegel, Akkumodul mit Zellen, Schalter. Die Schnittmenge muss leer sein.
- Selbsttest: Ein absichtlich zu großer Prüfkörper muss einen Treffer liefern.
- Standsicherheit: Das Lot des Schwerpunkts liegt mit ≥ 5 mm Abstand innerhalb der Sitzfläche (Massenliste im SCAD, wie in Stufe 2).
- Bilder: `assembly`, `exploded`, `section`.
- **Stufe 2 bleibt gleich:** `check.ps1` ist grün, und die STL-Dateien von Stufe 2 sind vor und nach dem Umbau gleich. Verglichen werden die sortierten Eckpunkt-Listen.

## Nicht in diesem Paket
- Akkuanzeige über GPIO 35.
- Firmware der Kamera.
