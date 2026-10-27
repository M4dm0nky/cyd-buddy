# Kamera-Halter + Verkabelung Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Kameramodul und XIAO-Stapel werden in Stufe 2 und im Roboter sicher gehalten, das Linsenloch hat die richtige Größe, und die Verkabelung CYD↔XIAO ist dokumentiert.

**Architecture:**
- Die neue Datei `case/xiao_cam_common.scad` enthält Parameter, die Tasche, den Riegel, die Wiege und die Prüfkörper.
- Beide Gehäusedateien binden sie per `include` ein. Sie ersetzt dort das alte Linsenloch und die Klemmrippen.
- Die Prüfskripte bekommen Tests für die Kamera.
- Die Doku liegt in `docs/verkabelung.md` + `docs/verkabelung.svg`.

**Tech Stack:** OpenSCAD 2021.01, PowerShell 5.1 (Skripte im PowerShell-Tool mit `& script.ps1`), SVG von Hand.

## Global Constraints
- Spec: `docs/superpowers/specs/2026-10-27-kamera-halter-design.md`. Fakten aus `.superpowers/sdd/2026-10-27-roboter-druckteile/kamera-recherche.md`.
- PETG ohne Stützen. Überhang ≤ 45°, Brücken ≤ 15 mm, Wand ≥ 2 mm (Ausnahme: Riegel 1,2 mm), `fit = 0.3`, `fit_peg = 0.15`.
- Jede Maßvariable bekommt einen Herkunftskommentar `QUELLE:`/`GEMESSEN:`/`VORLÄUFIG`. Keine OpenSCAD-`WARNING`. Keine doppelte Zuweisung (Include-Reihenfolge wie bei `cyd_common.scad`).
- Linsenloch OV3660: Ø 6,7 mit 45° Fase außen. Kameratasche: Körper + 0,25 mm je Seite.
- Die Prüfskripte `case/check.ps1` und `case/check_roboter.ps1` sind am Ende jedes Tasks grün.
- Commit auf `feature/roboter-druckteile`, nicht pushen. `brag-output-*` nicht committen. Jede Commit-Nachricht endet mit `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Prozesse: Jeder OpenSCAD- und curl-Aufruf mit Zeitlimit. Am Ende läuft kein eigener Prozess mehr. Ollama-`python.exe` in Ruhe lassen. Kein `python -` per Heredoc.

---

### Task K1: `xiao_cam_common.scad` + Einbau in Stufe 2

**Files:**
- Create: `case/xiao_cam_common.scad`
- Modify: `case/madbuddy_case.scad`, `case/check.ps1`, `case/README.md` (Teile, Druck, Montage der Kamera)

**Interfaces (Produces):**
- Module, alle in einem lokalen System: Frontwand-Innenseite bei Z = 0, Blickrichtung +Z nach außen, Mitte Linse = Ursprung.
  - `cam_pocket_add(wall_t)` (positiv)
  - `cam_pocket_cut(wall_t)` (negativ)
  - `cam_clip()`
  - `xiao_cradle_add()`
  - `xiao_cradle_cut()`
  - `cam_dummy()`
  - `xiao_stack_dummy()`
  - `cam_fov_cone(L)`
- `cam_clip_place()`: Riegel in Einbaulage.
- `xiao_offset`: Lage der Wiege relativ zur Linse.
- Mit `xiao_offset` gilt: Der FPC-Stecker liegt ≤ 16 mm Luftlinie von der Taschenkante entfernt.

- [ ] **Step 1: Prüfungen zuerst.**
  - `check.ps1`: Die Teileliste bekommt `cam_clip`. Kollision wie bisher, aber mit den neuen Prüfkörpern (`cam_dummy`, `xiao_stack_dummy`, `cam_fov_cone`) statt `xiao_dummy`/`lens_cone`.
  - Neuer `part="cam_selftest"`: Kamerakörper +0,6 mm muss Facetten liefern.
  - Neuer `part="clip_test"`: Schnittmenge Riegel ∩ Rahmen in Einbaulage > 0.
  - Ausführen → FAIL (Module fehlen).
- [ ] **Step 2: `xiao_cam_common.scad` schreiben** nach Spec, Abschnitt 1. Werte:
  - OV3660: Körper 8,0, Tiefe 5,3, Tubus Ø 6,2, Tubuslänge 2,7 (VORLÄUFIG), FPC 6,5 breit / 21,2 lang.
  - OV5640: 8,5 / 5,5 / Tubus 6,2 (VORLÄUFIG) / FPC 70,5.
  - XIAO: 21 × 17,8 × 15 (VORLÄUFIG), Platinendicke 1,2 (VORLÄUFIG).
  - Riegel: 1,2 dick, Übermaß 0,1.
  - FPC-Biegeradius ≥ 2.
- [ ] **Step 3: Einbau in `madbuddy_case.scad`.**
  - Linsenloch Ø 9, Klemmrippen und alte XIAO-Prüfkörper ersetzen.
  - Lage: Linse wie bisher mittig in der Stirn (`xiao_cy`). Die Wiege liegt dahinter so, dass das FPC passt.
  - Die Rückwand-Anschläge (`back_part`: Anschlagpfosten und Kabelkanal-Schienen) an die neue XIAO-Lage anpassen.
  - Kabelweg zu P1 (linke Schmalseite) und CN1 (untere Längskante) frei lassen. Bei Bedarf einen Kabelkanal vorsehen.
- [ ] **Step 4:**
  - `& case\check.ps1` muss grün sein. Gegenprobe `-Define window_shrink=2 -NoPng` muss FAIL liefern.
  - Bilder `madbuddy_schnitt.png` und `madbuddy_front.png` mit Read ansehen. Die Linse ist jetzt klein.
  - Zusätzlich ein Schnittbild durch die Kameratasche (Kamera `section` mit Schnitt durch X = 0) als `render/madbuddy_kamera_schnitt.png`.
- [ ] **Step 5: Commit** `feat(gehaeuse): Kamera-Tasche mit Riegel + XIAO-Wiege (Stufe 2), Linsenloch 6,7`.

### Task K2: Einbau im Roboter

**Files:** Modify: `case/madbuddy_roboter.scad`, `case/check_roboter.ps1`

**Interfaces:**
- Consumes: alles aus K1.
- Produces: `torso_front` mit Tasche 15° gekippt, neues Teil `cam_clip` in `$parts`.

- [ ] **Step 1:** `check_roboter.ps1` erweitern:
  - Kamera-Prüfkörper und Sichtkegel in `dummies()`.
  - Neue Parts `cam_selftest` und `clip_test`.
  - Ausführen → FAIL.
- [ ] **Step 2: Umbauen.**
  - `cam_mount()`/`lens_cut()` aus Task 3 durch `cam_pocket_*` und `xiao_cradle_*` ersetzen, 15° gekippt (`cam_tilt`).
  - Die Werkzeugkanäle aus Task 3 (X ±8, Y −1…−7) und das Kabelloch Ø 10 müssen frei bleiben.
  - Schraubendreher-Nachweise müssen weiter grün sein.
- [ ] **Step 3:**
  - `& case\check_roboter.ps1` grün, inkl. aller vorhandenen Selbsttests.
  - Schnittbild durch die Brust (`render/roboter_kamera_schnitt.png`) ansehen.
- [ ] **Step 4: Commit** `feat(roboter): Kamera-Tasche mit Riegel + XIAO-Wiege in der Brust`.

### Task K3: Verkabelungsanleitung

**Files:**
- Create: `docs/verkabelung.md`, `docs/verkabelung.svg`
- Modify: `README.md` (Stufe 2 und 3: Link, Verkabelungstabelle durch die neue ersetzen), Materialliste (Kabel: 2× „1,25 mm 4-Pin Kabel mit Stecker, einseitig offen“, statt „Steckertyp nicht belegt“)

- [ ] **Step 1:** `verkabelung.md` nach Spec, Abschnitt 3:
  - Tabelle mit Steckerbeschreibung, Lage am CYD und Pin-Zählung.
  - Ansicht XIAO von oben mit beiden Pinreihen.
  - Warnhinweise:
    - P1-TX/RX nicht benutzen.
    - CN1-3V3 nicht benutzen.
    - Pin 1 am eigenen Board prüfen.
    - XIAO vor dem Einbau flashen.
    - Diode bei gleichzeitigem USB am XIAO.
  - Schritt-für-Schritt-Anleitung:
    1. Kabel ablängen.
    2. Adern isolieren.
    3. An den XIAO löten.
    4. Prüfen mit Multimeter: kein Kurzschluss 5V–GND.
    5. Einstecken.
  - Je ein Abschnitt für Stufe 2 und Roboter: Kabellängen, Weg durch Hals und Oberkörper, Akku → Schalter → VIN und XIAO 5V.
- [ ] **Step 2:** `verkabelung.svg` von Hand zeichnen:
  - CYD-Rückseite schematisch, 86 × 50 im Maßstab, mit USB, SD, P1 und CN1, Lage laut Recherche.
  - XIAO, Leitungen farbig: rot 5V, schwarz GND, gelb IO27↔D6, grün IO22↔D7.
  - Beschriftung auf Deutsch.
  - SVG mit Read prüfen (als Bild: zuerst per OpenSCAD nicht möglich, deshalb die Struktur prüfen, und per `msedge --headless --screenshot` rendern, mit Zeitlimit, danach den Prozess beenden).
- [ ] **Step 3:** README anpassen, Links prüfen.
- [ ] **Step 4: Commit** `docs: Verkabelung CYD↔XIAO mit Grafik`.
