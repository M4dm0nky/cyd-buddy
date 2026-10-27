# Roboter-Druckteile Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Druckbare, steckbare, mehrfarbige Einzelteile für den MadBuddy-Roboter (Stufe 3), dazu Prüfskript und 3MF, ohne die Geometrie von Stufe 2 zu verändern.

**Architecture:**
- CYD-Maße und -Prüfkörper wandern in `case/cyd_common.scad`, das per `include` eingebunden wird.
- `case/madbuddy_case.scad` (Stufe 2) und die neue `case/madbuddy_roboter.scad` binden beide diese Datei ein.
- Der Roboter wählt das Teil über `part=` aus, wie Stufe 2.
- `case/check_roboter.ps1` exportiert alle Teile und prüft Kollision, Selbsttest und Stand. Danach rendert es die Bilder.
- `case/tools/stl2_3mf.py` packt die Teile in zwei 3MF-Dateien.

**Tech Stack:** OpenSCAD 2021.01 (`C:\Program Files\OpenSCAD\openscad.com`), PowerShell 5.1, Python 3 (`python -I`, kein `python -` per Heredoc).

## Global Constraints
- Spec: `docs/superpowers/specs/2026-10-27-roboter-druckteile-design.md`. Form und Maße aus `case/konzept/roboter_konzept.scad`, Strom aus `docs/superpowers/specs/2026-10-26-roboter-gehaeuse-design.md`.
- PETG, ohne Stützen, Überhang ≤ 45°, Brücken ≤ 15 mm, Wand ≥ 2 mm, `fit = 0.3`, `fit_peg = 0.15`.
- Jede Maßvariable trägt einen Kommentar mit Herkunft: `QUELLE:`, `GEMESSEN:` oder `VORLÄUFIG (Datenblatt)`. Code und Kommentare sind deutsch und ASCII-nah, wie in `madbuddy_case.scad` (ae/oe/ue in Kommentaren erlaubt).
- **Stufe 2 darf sich nicht ändern.** `case/check.ps1` muss grün bleiben, und die sortierten Eckpunkte von `madbuddy_front/back/frame_test.stl` müssen vorher und nachher gleich sein.
- OpenSCAD darf beim Export keine `WARNING` ausgeben. Eine Variable zweimal zuzuweisen erzeugt eine Warnung. Deshalb definiert `cyd_common.scad` **keine** gehäuseabhängigen Werte (`front_t`, `glass_gap`, `fit`, `wall`, `disp_x`). Diese Werte setzt die einbindende Datei.
- Prozesse: Jeder OpenSCAD-Aufruf bekommt ein Zeitlimit wie `Run-Scad` in `check.ps1`. Am Ende darf kein `openscad`- oder `python`-Prozess mehr laufen.
- Nicht pushen. Committen nur auf Branch `main` im Repo `C:\Dropbox\Incomming\github\3zoll`. Der Ordner `brag-output-*` wird nicht mit committet.
- Commit-Nachrichten enden mit `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

---

### Task 1: `cyd_common.scad` herauslösen (Stufe 2 bleibt gleich)

**Files:**
- Create: `case/cyd_common.scad`, `case/tools/stl_fingerprint.ps1`
- Modify: `case/madbuddy_case.scad`

**Interfaces:**
- Produces: In `cyd_common.scad` stehen:
  - Parameter des CYD: `pcb_*`, `hole_*`, `glass_*`, `disp_h`, `lcd_len`, `aa_*`, `act_*`, `win_off_*`, `win_margin`, `win_r`, `view_angle`, `win_bevel`, `window_shrink`, `rear_*` (außer `rear_extra`), USB/BTN/SD-Werte
  - abgeleitete Werte: `pcb_x0`, `pcb_x1`, `glass_x1`, `btn_x`, `win_w`, `win_h`, `hole_pts`, `z_glass_top`, `z_glass_bot`, `z_pcb_top`, `z_pcb_bot`, `z_rear`, `usb_list`, `usb_zc`, `sd_x`, `sd_zc`, `edge_y()`
  - Module: `rrect()`, `window_cut()`, `pcb_dummy()`, `glass_dummy()`, `rear_dummy()`, `sockets_dummy()`, `buttons_dummy()`, `sd_dummy()`, `active_cone()`
  - Erwartete Werte aus der einbindenden Datei: `front_t`, `glass_gap`, `fit`, `disp_x`
- Gehäusespezifisches bleibt in `madbuddy_case.scad`. Dazu gehören `rear_extra`, `hx_in`, Dome, XIAO, `plugs_dummy` (braucht `hx`) und `card_dummy` (braucht `hy`). Was ein Modul braucht, das von `hx`/`hy` abhängt, bleibt ebenfalls dort.

- [ ] **Step 1: Fingerprint-Werkzeug schreiben** `case/tools/stl_fingerprint.ps1`. Es liest eine ASCII-STL, rundet jede `vertex`-Zeile auf 4 Nachkommastellen, sortiert und entfernt doppelte Einträge, bildet einen SHA256 über den Text und gibt `<datei> <hash> <anzahl>` aus.
```powershell
param([Parameter(Mandatory)][string[]]$Files)
$inv = [Globalization.CultureInfo]::InvariantCulture
foreach ($f in $Files) {
  $set = New-Object 'System.Collections.Generic.SortedSet[string]'
  foreach ($m in [regex]::Matches((Get-Content $f -Raw), 'vertex\s+(\S+)\s+(\S+)\s+(\S+)')) {
    $v = 1..3 | ForEach-Object { [double]::Parse($m.Groups[$_].Value, $inv).ToString('F4', $inv) }
    [void]$set.Add(($v -join ' '))
  }
  $txt = ($set -join "`n")
  $h = [BitConverter]::ToString((New-Object Security.Cryptography.SHA256Managed).ComputeHash([Text.Encoding]::UTF8.GetBytes($txt))).Replace('-', '')
  "{0} {1} {2}" -f (Split-Path $f -Leaf), $h, $set.Count
}
```
- [ ] **Step 2: Ausgangswert festhalten:** `powershell -File case\tools\stl_fingerprint.ps1 case\stl\madbuddy_front.stl case\stl\madbuddy_back.stl case\stl\madbuddy_frame_test.stl > $env:TEMP\fp_before.txt`. Diese STL-Dateien sind der Commit-Stand `d17d7d2`.
- [ ] **Step 3: Herauslösen.** Blöcke ausschneiden und `include <cyd_common.scad>` am Anfang von `madbuddy_case.scad` einfügen (nach dem Kopfkommentar und `part = ...`). In `madbuddy_case.scad` bleibt `disp_x = -1.0;` mit Kommentar stehen.
- [ ] **Step 4: Prüfen:** `powershell -File case\check.ps1 -NoPng`. Erwartet `== ALLES GRUEN ==`. Danach erneut den Fingerprint bilden, in `fp_after.txt` schreiben und mit `Compare-Object (gc fp_before.txt) (gc fp_after.txt)` vergleichen. Erwartet: keine Ausgabe. Falls nicht: Ursache suchen, nicht weitermachen.
- [ ] **Step 5: Gegenprobe:** `powershell -File case\check.ps1 -Define window_shrink=2 -NoPng` muss FAIL melden (Kollision).
- [ ] **Step 6: Commit** `refactor(gehaeuse): CYD-Maße in cyd_common.scad (Stufe 2 unverändert)`. Mit committen: `case/cyd_common.scad`, `case/madbuddy_case.scad`, `case/tools/stl_fingerprint.ps1` und die STL-Dateien nur, falls `check.ps1` sie neu geschrieben hat. Weil der Fingerprint gleich ist, darf dabei kein Geometrie-Unterschied entstehen.

### Task 2: Roboter-Kopf und Prüfgerüst

**Files:**
- Create: `case/madbuddy_roboter.scad`, `case/check_roboter.ps1`

**Interfaces:**
- Consumes: alles aus `cyd_common.scad` (Task 1)
- Produces: in `madbuddy_roboter.scad`:
  - `part` mit den Werten `head_front`, `bezel`, `head_back`, `ear`, `assembly`, `exploded`, `section`, `collision`, `selftest`, `info`
  - Module `head_front()`, `bezel()`, `head_back()`, `ear()` in **Konstruktionslage**. Gesichtsebene Z=0 wie in Stufe 2, Ursprung = Mitte der aktiven Fläche, `disp_x = 0`.
  - Modul `place_head()`: Transform von der Kopf- in die Roboter-Weltlage. Z oben, das Gesicht blickt nach −Y, um `head_tilt = 8` nach hinten geneigt.
  - `dummies()` für alle Prüfkörper in Weltlage
  - `shells()` für alle Gehäuseteile in Weltlage
  - `check_roboter.ps1`: Parameter `-Define`, `-NoPng` wie `check.ps1`, Liste `$parts`, die spätere Tasks erweitern
- Gesichtsebene, Fenster, CYD-Halterung (Andrückfüße mit Zentrierstift wie in `madbuddy_case.scad` `back_part`), USB-C-Öffnung und BOOT/RESET übernehmen die Logik aus Stufe 2.

- [ ] **Step 1: Prüfskript zuerst.** `case/check_roboter.ps1` aus `check.ps1` ableiten. Danach:
  - `$scad` zeigt auf `madbuddy_roboter.scad`.
  - STL-Ausgabe nach `case/stl/roboter_<part>.stl` (bei `-Define` nach `%TEMP%`).
  - Prüfschritte: STL-Export aller `$parts`, `collision` muss leer sein, `selftest` muss treffen, `info` muss `STAND_MIN_MARGIN=<x>` ≥ 5 ausgeben (nur, wenn `seat` existiert, sonst übersprungen mit Hinweis).
  - PNGs `roboter_assembly`, `roboter_exploded`, `roboter_section`.
  - Aufräumen der OpenSCAD-Prozesse wie in `check.ps1`.
- [ ] **Step 2: Fehlschlag zeigen:** `powershell -File case\check_roboter.ps1 -NoPng` → FAIL, weil die Datei fehlt.
- [ ] **Step 3: Kopf bauen.**
  - Grundform: Kopfmaß nach Konzept 114 × 84, Tiefe aus Stufe 2 (Display, Platine, Rückseitenbauteile `rear_h`, Luft). Rundum verrundet (Konzept `rbox3`), vorn umlaufend 45°-Fase statt Kugelrundung, damit Gesicht nach unten ohne Stützen druckbar ist.
  - `head_front` enthält: Mulde für die Blende (Tiefe 0,8, Spiel `fit_peg`), 4 Dome M3 (Kernloch 2,6), Ohrlöcher seitlich (Ø 6 + `fit_peg`, 6 tief, mit Abflachung) und 4 erhabene Zierschrauben (Ø 4, 0,6 hoch) außerhalb der Blende.
  - `bezel`: Rahmen 1,2 mm dick, Außenkontur = Mulde, Innenkontur = Fensterform von `window_cut()` auf der Vorderseite. Die Blende liegt in der Mulde, das Fenster schneidet durch beide Teile.
  - `head_back`: Falz wie in Stufe 2, M3-Senkung, USB-C-Öffnung, BOOT/RESET, unten der Halssockel (Bohrung für den Halsflansch mit 2 M3-Kernlöchern), Kabelloch ≥ 10.
  - `ear`: Scheibe Ø 30 × 10 mit Zapfen Ø 6 × 6 mit Abflachung und 0,5er Fase, erhabene Zierschraube Ø 8.
  - Druckausrichtung im `part`-Zweig: `head_front` und `bezel` mit dem Gesicht nach unten, `head_back` außen nach unten, `ear` flach.
  - Massen-Einträge für den Kopf in einer Liste `masses` wie in Stufe 2.
- [ ] **Step 4: Prüfen:** `check_roboter.ps1 -NoPng`. Mit `$parts = head_front, bezel, head_back, ear` muss alles grün sein, die Kollision leer und der Selbsttest ein Treffer. Gegenprobe: `-Define window_shrink=2` muss FAIL liefern.
- [ ] **Step 5: Commit** `feat(roboter): Kopf (Front, Blende, Rückwand, Ohren) + Prüfskript`.

### Task 3: Oberkörper, Hals, Arme, Hände

**Files:** Modify: `case/madbuddy_roboter.scad`, `case/check_roboter.ps1` (`$parts` erweitern)

**Interfaces:**
- Consumes: `place_head()`, `dummies()`, `shells()`, `masses` aus Task 2
- Produces:
  - `torso_front()`, `torso_back()`, `neck()`, `arm()`, `hand()`
  - `place_torso()`, `torso_origin` (Mitte der Unterkante des Oberkörpers in Weltlage, darauf baut Task 4 auf)
  - `xiao_dummy_r()`, `lens_cone_r()` in Weltlage

- [ ] **Step 1:** `$parts` um `torso_front, torso_back, neck, arm, hand` erweitern und `check_roboter.ps1 -NoPng` laufen lassen → FAIL (Teile fehlen).
- [ ] **Step 2: Bauen.**
  - Oberkörper 64 × 44 × 48, verrundet, in vorn und hinten geteilt (Teilungsebene senkrecht, M3 an 4 Domen).
  - XIAO (21 × 17,8 × 15, VORLÄUFIG) hinter der Brust. Linse Ø 8, Loch Ø 9, um `cam_tilt = 15` nach oben, mit Klemmrippen und Anschlag wie in Stufe 2.
  - Oben eine Aufnahme für den Hals, unten eine Steckkante und 4 M3-Kernlöcher für die Verbindung zu `seat`. Kabeldurchführung unten ≥ 10 mm.
  - Hals: Rohr Ø 22, Innen-Ø ≥ 12, Flansch oben und unten mit je 2 Durchgangslöchern Ø 3,4. Druck stehend.
  - Arm: Kugel R 8,5 mit Abflachung unten, Zapfen Ø 6 in die Schulter, Arm-Hull nach Konzept, Zapfen Ø 5 für die Hand. Teilen, falls er nicht ohne Stützen druckbar ist (z. B. längs halbiert, verstiftet). Gespiegelt für links/rechts über `side = 1|-1`.
  - Hand: abgeflachte Ellipse nach Konzept, mit Zapfenloch.
  - Massen ergänzen.
- [ ] **Step 3: Prüfen:** `check_roboter.ps1 -NoPng` grün (Kollision mit XIAO und Linsenkegel leer).
- [ ] **Step 4: Commit** `feat(roboter): Oberkörper, Hals, Arme, Hände`.

### Task 4: Po/Beine mit Akkufach, Boden, Füße, Standsicherheit

**Files:** Modify: `case/madbuddy_roboter.scad`, `case/check_roboter.ps1`

**Interfaces:**
- Consumes: `torso_origin`, `masses`, `dummies()`
- Produces: `seat()`, `bottom()`, `foot()`, `battery_dummy()`, `switch_dummy()`, `info` mit `STAND_MIN_MARGIN=<mm>`

- [ ] **Step 1: Datenblatt holen.** Maße für DFR0969 recherchieren: DFRobot-Wiki bzw. Produktseite https://www.dfrobot.com/product-2578.html, Abmessungszeichnung, Foto. Gesucht sind Platine L×B×Dicke, Höhe mit Zellen, Lage von USB-C, Micro-USB, HOLD/NORMAL-Schalter, 5V/GND-Ausgang und LED. Jeden Wert mit `VORLÄUFIG (Datenblatt)` und Quelle als Kommentar eintragen. Was nicht zu finden ist, als `VORLÄUFIG (Schätzung)` mit großzügiger Aussparung.
- [ ] **Step 2:** `$parts` um `seat, bottom, foot` erweitern und den Stand-Schritt aktivieren → FAIL.
- [ ] **Step 3: Bauen.**
  - `seat`: Form nach Konzept `legs()` ohne Füße, unten offen, Akkufach quer mit `batt_clear = 1.0`.
  - Oben eine Steckaufnahme für den Oberkörper mit 4 Durchgangslöchern und Senkung von unten. Kabelloch nach oben.
  - Vorn an jedem Bein ein Zapfen Ø 8 für den Fuß.
  - 4 Dome für `bottom`.
  - `bottom`: flache Platte (≥ 2,4) mit Falz, mit folgenden Öffnungen:
    - USB-C-Ladebuchse, 1 mm Zugabe rundum
    - LED-Loch Ø 3
    - Aussparung für SS12D00 (VORLÄUFIG), Hebel nach unten bedienbar
    - Öffnung für HOLD/NORMAL
    - 4× M3 mit Senkung
  - Die Unterseite ist die Standfläche.
  - `foot`: nach Konzept, Zapfenloch, schwarz.
  - Massen eintragen: Akku mit 2 Zellen ca. 100 g ganz unten. Daraus `STAND_MIN_MARGIN` berechnen, also den kleinsten Abstand des Schwerpunkt-Lots zum Rand der Aufstandsfläche (Projektion von `seat`+`foot` auf Z=0, vereinfacht als umschließendes Rechteck der Aufstandspunkte).
- [ ] **Step 4: Prüfen:** `check_roboter.ps1` (mit PNGs) grün. Bilder ansehen:
  - Assembly ähnelt `render/roboter_vorn.png`.
  - Exploded zeigt alle Teile getrennt.
  - Der Schnitt zeigt Akku, XIAO und CYD ohne Überschneidung.
- [ ] **Step 5: Commit** `feat(roboter): Po/Beine mit Akkufach, Boden, Füße, Standsicherheit`.

### Task 5: 3MF-Dateien und Doku

**Files:**
- Create: `case/tools/stl2_3mf.py` (Kopie aus `C:\Dropbox\Incomming\github\HA_BUDDY\gehaeuse\tools\stl2_3mf.py`, erweitert), `case/3mf/roboter_teile.3mf`, `case/3mf/roboter_ams.3mf`
- Modify: `case/check_roboter.ps1` (3MF-Schritt), `case/README.md`, `README.md` (Stufe 3: Stand + Teileliste + Druck), `docs/superpowers/specs/2026-10-26-roboter-gehaeuse-design.md` (Verweis auf die neue Spec)

**Interfaces:**
- Consumes: `case/stl/roboter_*.stl`
- Produces:
  - `stl2_3mf.py <ziel.3mf> [--group NAME] spec...`. Mit `--group` werden alle Specs zu **einem** Objekt mit Bauteilen (`<components>`) zusammengefasst, ohne `--group` wie bisher getrennt.
  - Neuer Teile-Modus `head_front_ams` in `madbuddy_roboter.scad`: `head_front` mit eingesetzter `bezel` und Zierschrauben als getrennte Körper. Dazu die Ausgabe `part=bezel_inlay` und `part=screws_inlay` in **derselben** Lage wie `head_front`, damit die Teile zueinander passen.

- [ ] **Step 1: Erweitern und testen.**
  - `--group` einbauen. Bei der Gruppe werden die Teile **gemeinsam** verschoben (gemeinsame Bounding-Box), nicht jedes Teil für sich zentriert.
  - Test: `python -I case\tools\stl2_3mf.py %TEMP%\t.3mf --group kopf a=case\stl\roboter_head_front.stl b=case\stl\roboter_bezel.stl`. Danach die Zip-Datei öffnen und prüfen, dass `3D/3dmodel.model` genau ein `<item>` und ein Objekt mit 2 `<component>` enthält. Die Prüfung macht ein PowerShell-Einzeiler mit `[IO.Compression.ZipFile]`.
- [ ] **Step 2:** In `check_roboter.ps1` beide 3MF-Dateien erzeugen (ASCII-STL, ist bei OpenSCAD 2021 Standard) und prüfen, dass sie größer als 10 kB sind.
  - `roboter_teile.3mf`: alle Teile, Ohr/Arm/Hand/Fuß je 2×, Arm gespiegelt, nebeneinander angeordnet.
  - `roboter_ams.3mf`: Gruppe Kopf (head_front + bezel_inlay + screws_inlay) plus alle übrigen Teile einzeln.
- [ ] **Step 3: Doku.**
  - `case/README.md` bekommt einen Abschnitt „Variante 2 Roboter“ mit:
    - Teiletabelle mit Farbe und Menge
    - Druckhinweisen
    - Schraubenliste (Anzahl M3 nach Länge zählen)
    - Montagereihenfolge
    - offenen Punkten (Akkumaße VORLÄUFIG)
  - `README.md`, Stufe 3: Der Stand wird „druckbar, Maße des Akkumoduls vorläufig“. Die Schraubenzeile bekommt die echte Anzahl. Dazu ein Link auf die 3MF und die STL-Dateien.
- [ ] **Step 4: Prüfen:** `check_roboter.ps1` und `check.ps1` sind beide grün.
- [ ] **Step 5: Commit** `feat(roboter): 3MF (Einzelteile + AMS), Doku`.
