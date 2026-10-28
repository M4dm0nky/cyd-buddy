# Kamera-Halter und Verkabelung (Stufe 2 und Roboter)

Stand 2026-10-27. Anlass ist die Rückmeldung des Nutzers:
- Die Kamera hat keinen Halter. Sie hängt lose am Flachbandkabel.
- Das Linsenloch (Ø 9) ist viel zu groß.
- Es fehlt eine Anleitung, wie XIAO und CYD verbunden werden.

Quellen: `.superpowers/sdd/2026-10-27-roboter-druckteile/kamera-recherche.md` (Runde 1 + 2). Die Maße stammen aus Seeed-Zeichnungen.

## Fakten (belegt)
- **Kameramodul im Lieferumfang:** OV3660 laut Seeed-Bild. Den Aufdruck am eigenen Teil prüfen.
  - Körper 8,0 × 8,0 mm, Gesamttiefe 5,3 mm, Tubus Ø 6,2.
  - FPC 21,2 × 6,5 mm, Versteifung am Ende 12,5 × 4,0, 24 Pins im 0,5-mm-Raster.
  - FOV diagonal 102°, horizontal 85°.
- **Alternative OV5640:** Körper 8,5 × 8,5, ca. 5,5 tief, FPC 70,5.
- **Einbauzustand ab Werk:** Das FPC steckt im Klappstecker auf der Sense-Platine (am Ende gegenüber der USB-Buchse). Das Modul liegt lose, es gibt keinen Halter.
- **XIAO-Pins:**
  - Reihe A: 5V (Pin 1, an USB), GND (Pin 2), 3V3, D10, D9, D8, D7 (Pin 7, gegenüber USB).
  - Reihe B: D0 bis D6, D6 liegt gegenüber USB.
- **CYD:**
  - **P1** (1,25 mm, 4-polig, linke Schmalseite neben der USB-Buchse): VIN, TX, RX, GND.
    - VIN ist ein 5-V-Eingang über einen P-FET mit Verpolschutz.
    - USB-5V ist über eine Diode entkoppelt.
  - **CN1** (1,25 mm, 4-polig, untere Längskante): 3V3, IO27, IO22, GND.
  - Backlight auf GPIO 21, laut `src/config.h` am Gerät bestätigt. Damit sind GPIO 22 und 27 frei.

## Entscheidungen

### 1. Gemeinsame Datei `case/xiao_cam_common.scad`
Stufe 2 (`madbuddy_case.scad`) und Roboter (`madbuddy_roboter.scad`) binden sie per `include` ein. Sie enthält:
- **Parameter** mit Herkunft, ausgewählt über `cam_type = "OV3660"` bzw. `"OV5640"`.
  - Kamera: `cam_body`, `cam_depth`, `cam_tube_d`, `cam_tube_len` (VORLÄUFIG 2,7), `cam_fpc_w`.
  - XIAO-Stapel: `xiao_w` 21, `xiao_l` 17,8, `xiao_stack` 15 (VORLÄUFIG), `xiao_pcb_t` (VORLÄUFIG 1,2).
- **`cam_pocket(wall_t)`:** positive Geometrie an der Innenseite einer Frontwand. Die Tasche entsteht daraus mit `difference`.
  - **Linsenloch:** Ø `cam_tube_d + 0,5` (OV3660: 6,7), nach außen mit 45° gefast. Ist der Tubus länger als die Wand, wird die Wand um die Linse innen verstärkt, bis die Linse 0,3 mm hinter der Außenfläche endet.
  - **Rahmen:** Er umfasst den Körper mit `cam_body + 2 × 0,25` Spiel. Höhe = Körpertiefe ohne Tubus.
  - **FPC-Schlitz:** Eine Seite ist für das FPC offen, Schlitzbreite `cam_fpc_w + 0,7`.
  - **Führungsnuten:** Zwei seitliche Nuten nehmen den Riegel auf.
- **`cam_clip()`:** eigenes Druckteil, der **Kamera-Riegel**.
  - U-förmige Platte, 1,2 mm dick. Sie wird seitlich in die Nuten geschoben und drückt den Kamerakörper gegen die Wand.
  - Eine Aussparung lässt das FPC durch, mit Biegeradius ≥ 2 mm.
  - Klemmung über 0,1 mm Übermaß, kein Kleber. Optional ein dünnes Klebepad auf der Rückseite.
- **`xiao_cradle()`:** Halter für den XIAO-Stapel, Druck ohne Stützen.
  - Bodenplatte mit zwei Stirnanschlägen: am USB-Ende und am Ende gegenüber.
  - Ein Steg über der Sense-Platine bzw. eine Klemmlasche hält den Stapel. Er wird von oben eingelegt.
  - **Die Längsseiten bleiben frei**, weil dort die angelöteten Drähte sitzen. Daher keine Schienen an den Längsseiten.
  - Lage: Das Ende mit dem Kamerastecker zeigt zur Kamera. Der Abstand ist so gewählt, dass das 21,2-mm-FPC mit einer lockeren Schleife reicht, also Stecker zu Kameratasche ≤ 16 mm Luftlinie.
- **Prüfkörper:** `cam_dummy()` (Körper, Tubus, FPC-Band), `xiao_dummy()` (Stapel), `cam_fov_cone()` (Sichtkegel 102° diagonal bzw. 85° horizontal, 30 mm lang; Roboter: 26 mm, weil die Füße ab ca. 27 mm unten im Bild liegen; Entscheidung 2026-10-28).
  - Der Kegel darf das Gehäuse nicht schneiden. Prüfung mit `collision`.

### 2. Einbau
- **Stufe 2:**
  - `front` (Stirn) bekommt `cam_pocket` und `xiao_cradle` statt Linsenloch Ø 9 und Klemmrippen.
  - Neues Druckteil `cam_clip`.
  - Die Bauform von Stufe 2 bleibt sonst gleich.
  - Wer `front` schon gedruckt hat, muss es neu drucken.
- **Roboter:**
  - `torso_front` bekommt `cam_pocket` in der Brust, 15° gekippt, dazu `xiao_cradle`.
  - Ersatz für die einfachen Platzhalter aus Task 3 (`cam_mount()`, `lens_cut()`).
  - Neues Druckteil `cam_clip`.
- In beiden Prüfskripten gilt:
  - Kollision Kamera und XIAO gegen Gehäuse ist leer.
  - Der Sichtkegel ist frei.
  - Ein Selbsttest mit zu großem Kamerakörper muss eine Kollision melden.
  - Schnitttest: Der Riegel sitzt in den Nuten (Schnittmenge mit dem Rahmen > 0 bei 0,1 mm Übermaß).

### 3. Verkabelung (Doku `docs/verkabelung.md` + Grafik `docs/verkabelung.svg`)
Es werden 4 Leitungen verlegt. Die CYD-Seite wird per 1,25-mm-Stecker angeschlossen, die XIAO-Seite gelötet.

| CYD-Stecker | Pin | Signal | → XIAO | Pin |
|---|---|---|---|---|
| P1 | 1 VIN | 5 V | 5V | Reihe A, Pin 1 (an USB) |
| P1 | 4 GND | Masse | GND | Reihe A, Pin 2 |
| CN1 | 2 IO27 | RX des CYD | D6 (TX) | Reihe B, Pin 7 |
| CN1 | 3 IO22 | TX des CYD | D7 (RX) | Reihe A, Pin 7 |

- Unbelegt bleiben P1 TX/RX (UART0 zum USB-Chip, **nicht** benutzen), CN1 3V3 (der Regler reicht nicht für die Kamera) und CN1 GND. CN1 GND darf als zweite Masse mit an XIAO GND.
- Zwei fertige Kabel „1,25 mm 4-Pin“ mit einseitigem Stecker, die nicht benötigten Adern isolieren.
- Kabellänge: Stufe 2 ca. 60 mm, Roboter ca. 150 mm (durch den Hals).
- **Versorgung im Roboter:** Akku → Schalter → P1 VIN/GND und parallel XIAO 5V/GND. Laut Recherche ist der XIAO-5V-Pin als Eingang zulässig, mit Diode gegen USB-Rückspeisung empfohlen, wenn beide gleichzeitig hängen.
- **Grafik:** Lage der Stecker auf der CYD-Rückseite, XIAO-Pinreihen, Leitungsfarben. Positionen auf dem CYD sind aus dem Foto geschätzt, deshalb steht dabei: „Pin-1-Markierung am eigenen Board prüfen“.
- Die README verlinkt die Anleitung bei Stufe 2 und 3.

## Prüfung
- `check.ps1` und `check_roboter.ps1` sind grün, inklusive der neuen Kamera-Tests und Selbsttests.
- Bilder: Schnitt durch Kameratasche und XIAO in beiden Varianten, mit Read angesehen.

## Offen (Nutzer misst nach)
- XIAO-Platinendicke und Stapelhöhe mit Kamerastecker.
- Tubuslänge der Kamera.
- Lage von P1/CN1 an der 2-USB-Variante. Alle drei Werte sind Parameter.
