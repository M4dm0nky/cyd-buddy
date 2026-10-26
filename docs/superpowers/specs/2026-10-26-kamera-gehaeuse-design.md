# MadBuddy bekommt Augen: Kamera, Verkabelung, Gehäuse

Stand 2026-10-26. Branch `feature/kamera-gehaeuse`.

## Ziel
Der Buddy soll **sehen, ob jemand da ist und wo das Gesicht ist**: Die Augen folgen dir, er begrüßt dich und schläft ein, wenn du gehst. Am CYD wird kein Kamerabild gezeigt.

## Warum eine eigene Kamera-Platine
- Ein DVP-Kameramodul (OV2640/OV5640) braucht etwa 16 Pins und PSRAM.
- Am CYD sind nur GPIO 22, 27 und 35 frei (siehe `src/config.h`), und PSRAM fehlt.
- SPI-Kameras mit eigenem Bildspeicher (ArduCAM Mini) würden zwar gehen, kosten aber etwa 25–30 €, schaffen nur wenige Bilder pro Sekunde und belegen die letzten Pins.
- **Entscheidung:** Seeed **XIAO ESP32S3 Sense** (Kamera, 8 MB PSRAM, Mikrofon, 21 × 18 mm, ca. 15 €), bestellt **ohne Header**. Er erkennt Gesichter selbst und schickt nur Koordinaten.

## Verkabelung (4 Drähte)
| CYD | XIAO | Zweck |
|---|---|---|
| P1 VIN (5 V) | 5V | Strom (nicht den 3,3-V-Pin von CN1 nehmen, der Regler reicht nicht) |
| P1 GND | GND | Masse |
| CN1 GPIO 27 (RX2) | D6 / TX | Daten Kamera → CYD |
| CN1 GPIO 22 (TX2) | D7 / RX | Befehle CYD → Kamera |

- Beide Boards laufen mit 3,3-V-Logik, ein Pegelwandler ist nicht nötig.
- **Die Pinreihenfolge von P1/CN1 vor dem Löten am Foto des Boards prüfen.**
- Strom über USB-C nur mit Kabel USB-A auf USB-C (fehlende CC-Widerstände). Das Netzteil braucht ca. 1 A.

## Gehäuse „Kopf mit Stirn“
- Es gibt kein fertiges Gehäuse für CYD und XIAO zusammen (MakerWorld und Printables geprüft). Daher selbst gebaut: `case/madbuddy_case.scad`, parametrisch, Details in `case/README.md`.
- Form: abgerundeter Kopf, symmetrisch um das Display-Fenster, Linse mittig darüber in der Stirn, Fuß mit 15°.
- Nur USB-C ist nach außen geführt, Micro-USB bleibt zu (Schalter `open_micro_usb`). Das SD-Fach liegt unten, die Firmware nutzt es nicht.
- Maße: Platine, Löcher und Display aus der Originaldoku (witnessmenow/ESP32-Cheap-Yellow-Display, Datenblatt JC2432A028N). Die USB-Lage ist gemessen, der Rest im SCAD als VORLÄUFIG markiert.
- Druck: PETG (die Kamera wird warm), ohne Stützen. Erst `frame_test`, dann `front` und `back`.
- Prüfung: `case/check.ps1` (Kollision, Standfestigkeit, Selbsttest). Gegenprobe mit `window_shrink=2` muss fehlschlagen.

## Firmware
Eigene Spec: `2026-10-26-kamera-firmware-design.md`.
- Der USB-C des XIAO ist im Gehäuse nicht erreichbar, deshalb vor dem Einbau flashen und Updates per WLAN einspielen.
