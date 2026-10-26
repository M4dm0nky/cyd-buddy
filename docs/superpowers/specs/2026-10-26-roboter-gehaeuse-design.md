# MadBuddy als sitzender Roboter: Gehäuse-Variante 2 (mit Akku)

Stand 2026-10-26. Branch `feature/kamera-gehaeuse`.

## Zwei Varianten
| | Variante 1 „Kopf mit Stirn“ | Variante 2 „Roboter“ |
|---|---|---|
| Strom | USB-C am Kopf | Akku (2 × 18650), Laden per USB-C im Boden |
| Kamera | in der Stirn über dem Display | in der Brust |
| Spec | `2026-10-26-kamera-gehaeuse-design.md` | diese Datei |

Firmware, Verkabelung der Daten (UART) und Kamera-Funktionen sind bei beiden Varianten gleich (`2026-10-26-kamera-firmware-design.md`). Die Varianten unterscheiden sich nur durch Gehäuse und Stromversorgung.

## Form
Eine sitzende Roboterfigur im Kindchenschema: großer Kopf, kleiner Oberkörper, kurze dicke Beine.
- Konzeptbilder: `case/render/roboter_vorn.png`, `roboter_links.png`, `roboter_seite.png`
- Konzeptmodell: `case/konzept/roboter_konzept.scad`. Das ist eine Skizze, keine Druckteile.
- Außenmaße etwa 165 mm hoch, 135 mm breit (mit Ohren), 90 mm tief.

| Teil | Inhalt | Form |
|---|---|---|
| **Kopf** | CYD | Abgerundeter Kasten 114 × 84 × 32 mm, Display-Fenster wie in Variante 1. Ohren als runde Scheiben links und rechts mit Schraube. Vier angedeutete Eck-Schrauben auf der Front. Um 8° nach hinten geneigt, damit er zu dir hochschaut. Keine Stirn, weil die Kamera in die Brust wandert. |
| **Hals** | Kabeldurchführung | Hohles Rohr, Ø 22 mm. Steckt Kopf und Oberkörper zusammen. |
| **Oberkörper** | XIAO ESP32S3 Sense | Abgerundeter Kasten 64 × 44 × 48 mm. Linse in der Brust, 15° nach oben gekippt, weil das Gesicht höher ist als die Brust. Vier Schrauben-Punkte auf der Brust. Lüftungsschlitze auf dem Rücken. |
| **Arme** | – | Werden einzeln gedruckt und an der Schulter eingesteckt. Gelenkkugel an der Schulter, die Hand liegt auf dem Oberschenkel. |
| **Beine/Po** | Akkuhalter DFRobot DFR0969 | Zwei dicke, runde Oberschenkel, hinten zusammen etwa 112 × 58 × 32 mm. Darin liegt das Akkumodul (103 × 48 mm, mit Zellen etwa 25 mm hoch) quer. Kurze Stummelbeine ragen etwa 3 cm nach vorn, die Füße stehen nach oben. |

**Standsicherheit:** Der Akku (etwa 100 g) liegt ganz unten, und der Kopf bleibt in der Seitenansicht über der Sitzfläche. Das prüfe ich wie bei Variante 1 per Schwerpunkt-Rechnung im SCAD.

**Farben (Vorschlag für den Druck):** Kopf und Beine blau, Oberkörper und Arme orange, Ohren gelb, Füße und Hals schwarz. Getrennte Teile lassen sich ohne Farbwechsel drucken.

## Strom
- Das Akkumodul liefert 5 V. Von seinen 5-V-/GND-Stiften gehen zwei Leitungen durch den Po und den Hals: eine zu P1 (VIN/GND) am CYD, eine zum 5-V-/GND-Pin des XIAO.
- **Laden:** Die USB-C-Buchse des Moduls sitzt auf seiner Unterseite an der Kante. Im Boden des Pos kommt dort eine Aussparung hin, man steckt das Kabel also von unten ein. Die Lade-LED leuchtet durch ein kleines Loch im Boden.
- **Ein/Aus:** Ein kleiner Schiebeschalter im Boden trennt die 5-V-Leitung zu den Platinen.
- **HOLD/NORMAL** steht fest auf **HOLD**, sonst schaltet das Modul bei wenig Last ab. Der Schalter bleibt durch eine Öffnung im Boden erreichbar.
- **USB-C am Kopf:** Die Buchse des CYD bleibt zum Flashen erreichbar (Öffnung seitlich im Kopf). **Offen:** Am CYD-Schaltplan prüfen, ob USB-5V und VIN (P1) gegeneinander geschützt sind. Bis das geklärt ist, gilt: Zum Flashen per Kabel den Schalter auf Aus stellen.
- **Laufzeit:** etwa 7–9 h bei 0,4–0,5 A, geschätzt. Am Gerät messen.

## Was gleich bleibt wie bei Variante 1
- Display-Fenster, Lochbild und Halterung des CYD: gleiche Maße, gleiche Parameter. Der `frame_test` gilt für beide Varianten.
- Die Halterung des XIAO (Klemmrippen, Anschlag) wandert vom Kopf in den Oberkörper.
- Druck in PETG ohne Stützen, Wandstärke ≥ 2 mm, Passungsspiel `fit`.

## Umsetzung (eigener Plan, nach Variante 1)
- Eigene Datei `case/madbuddy_roboter.scad`. Display-Maße und Halterungen kommen aus einer gemeinsamen Datei (`case/cyd_common.scad`), die auch `madbuddy_case.scad` nutzt.
- Teile:
  - Kopf vorn und hinten
  - Oberkörper vorn und hinten
  - Po/Beine oben und unten (Boden mit Ladeloch und Schalter)
  - 2 Arme, 2 Ohren
- Gesteckt und mit M3 verschraubt, damit man an den Akku herankommt.
- Prüfung wie Variante 1 mit einem eigenen `check`: Kollision mit Platine, Display, XIAO, Akkumodul und Zellen, Linsen-Freiraum, Standsicherheit, Druck ohne Stützen.
- **Hängt an Teilen, die noch nicht da sind:** die genauen Maße des Akkumoduls (Lage von Buchse, Stiften und Schalter, am Foto bzw. Teil prüfen) und der Testdruck von Variante 1 (Fenster, Lochbild).

## Später (nicht in diesem Paket)
- Akkustand am Display: Spannungsteiler von der Zellspannung an GPIO 35 (frei, nur Eingang, analog). Der Buddy wird müde, wenn der Akku leer wird.
