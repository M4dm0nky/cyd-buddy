# MadBuddy Gehäuse "Kopf mit Stirn"

Parametrisches OpenSCAD-Gehäuse (OpenSCAD 2021.01) für das ESP32-2432S028 (CYD 2,8", Micro-USB + USB-C) und ein Seeed XIAO ESP32S3 Sense mit Kameraplatine (Linse in der Stirn, schaut nach vorn).

Datei: `madbuddy_case.scad`. Teil wählen mit `-D 'part="front"'` (`front`, `back`, `frame_test`, `assembly`, `section`, `collision`, `info`).
Alle Maße stehen oben in der Datei mit Kommentar: `QUELLE: Originalzeichnung`, `GEMESSEN` oder `VORLÄUFIG`.

## Teile

| Teil | Datei | Inhalt |
|---|---|---|
| front | `stl/madbuddy_front.stl` | Kopfschale mit Display-Fenster (nach vorn angefast), Stirn, Linsenloch, Klemmrippen für den XIAO, 4 Schraubdome (M3, von hinten) |
| back | `stl/madbuddy_back.stl` | Rückwand mit Falz, USB-C-Öffnung, SD-Aussparung, BOOT/RESET-Fenster, Lüftungsschlitze, Platinen-Andrückfüße mit Zentrierstift, Kabelkanal, Fuß mit 15° Neigung |
| frame_test | `stl/madbuddy_frame_test.stl` | flacher Streifen (2 mm Front + Display-Auflage) zum Prüfen von Fenstermaß, Glas-Sitz und Lochbild in ca. 20 min |
| assembly / section | nur Ansicht (`render/*.png`) | alles mit farbigen Prüfkörpern |

## Druck (PETG)

- 0,2 mm Schichthöhe, 3 Wände, ohne Stützen. Die STL-Dateien liegen schon in Druckausrichtung.
- front: Gesicht nach unten auf dem Bett (Fenster und Linsenloch sind Durchbrüche, Fenster-Fase ist ein Überhang von 35°).
- back: Rückseite (Außenfläche) liegt flach unten, der Fuß hat nur 15° Überhang.
- frame_test: Gesicht nach unten.
- Schrauben: 4× M3×8 (Senkung hinten, Kernloch 2,6 mm in den Domen; bei PETG selbstschneidend).
- Mindestwand 2,4 mm, Passungsspiel `fit = 0,3 mm` (Variable).

## Prüfung

`& case\check.ps1` (aus PowerShell im Repo-Ordner) führt alles aus (STL ohne Warnungen, Standsicherheit, Standebene, Kollision gegen alle Prüfkörper inkl. Linsen-Freiraum und aktiver Fläche mit Sichtkegel, Selbsttest der Kollisionsprüfung, PNG-Renders). Exitcode ungleich 0 bei jedem Fehler.
Gegenprobe: `check.ps1 -Define window_shrink=2 -NoPng` muss fehlschlagen (Fenster 2 mm schmaler).

## Maße

- Aus Originalzeichnung (`QUELLE`): Platine 86 × 50, Eckradius 4, Löcher Ø3,2 mit 4,0 mm Randabstand, Displaymodul 69,2 × 50 × 2,3, LCD-Glas 64,75, Fenster = AA + 0,3 mm je Seite. Die Zeichnungs-AA (57,6 × 43,2, mittig im Glas) passte nicht.
- Gemessen (2026-10-26, nach Testdruck): bespielte Fläche 58,8 mm breit (Fenster rechnet mit 59), beginnt 10 mm nach der linken Platinenkante (Blick von vorn). Höhe 44,3 mm ist daraus hochgerechnet (VORLÄUFIG).
- Gemessen: USB-Buchsen-Lage entlang der Schmalseite (Micro ab 9,67 mm, USB-C 17,88 bis 27,32 mm, von der Kante mit dem SD-Slot).
- VORLÄUFIG (Datenblatt/Schätzung): Platinendicke, Displayhöhe über Platine (4,5), Micro-USB-Breite, alle Höhen (z) der Buchsen/Stecker, SD-Slot (Länge, Lage, Höhe), BOOT/RESET-Lage, Rückseitenbauteile (8 mm, Randstreifen), XIAO-Stapel (21 × 17,8 × 15), Linse, Sichtwinkel, Wandöffnungen, Schwerpunkt-Massen.

## Offene Punkte

- Micro-USB hat standardmäßig keine Wandöffnung (`open_micro_usb = false`), Innenfreiraum bleibt.
- SD-Slot liegt in der Konstruktion an der unteren Längskante; dort steht der Kopf auf dem Fuß, die Karte lässt sich dann nur bei angehobenem Gerät stecken. Die Firmware nutzt die SD-Karte nicht (geprüft 2026-10-26), daher bleibt das so.
- Standsicherheit ist eine Schätzung (Massenliste in der SCAD). Ohne die Zusatztiefe `rear_extra = 10` liegt das Lot bei 15° praktisch auf der Kippkante.
- XIAO-USB-C ist im Gehäuse nicht zugänglich (nur Programmierung vor dem Einbau).
- Fenster, Lochbild und Glas-Sitz zuerst mit `frame_test` prüfen. **Stand 2026-10-26: 1. Testdruck: Lochbild passt, Fenster lag falsch, korrigiert, 2. Testdruck steht aus.**
- Strom über den USB-C des CYD nur mit Kabel USB-A auf USB-C (keine CC-Widerstände). Das Netzteil braucht ca. 1 A, weil die Kamera mitversorgt wird.

## Variante 2: sitzender Roboter mit Akku (Konzept)

Wenn der Buddy mit Akku laufen soll, kommt diese Form: Roboterkopf mit dem CYD, Oberkörper mit der Kamera, dicke Stummelbeine mit dem Akkuhalter DFRobot DFR0969 (2 × 18650), Laden per USB-C im Boden.
Bisher gibt es nur ein Konzept: `konzept/roboter_konzept.scad` und `render/roboter_*.png`. Spec: `docs/superpowers/specs/2026-10-26-roboter-gehaeuse-design.md`.
