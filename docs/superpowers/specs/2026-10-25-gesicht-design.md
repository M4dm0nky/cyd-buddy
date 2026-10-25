# CYD-Buddy – Gesicht mit Brauen, Pupillen, Nase, Mund (Design)

Stand 2026-10-25, freigegeben vom Nutzer (Stil A „Roboter“, alle 22 Ausdrücke).
Vorlage: `docs/superpowers/mockups/2026-10-25-gesicht-ausdruecke.html` (+ `.png`). **Die Reglerwerte in der Liste `E` dieser Datei sind die Soll-Werte** der Ausdrücke; die Funktion `face()` dort ist die Soll-Zeichenvorschrift.

## Ziel
Das Augenpaar wird zu einem einfarbigen Gesicht in Stimmungsfarbe: Augen (wie bisher, mit Lidern) + dunkle Pupillen mit Glanzpunkt, Brauen-Striche, Nasenpunkt, Mund. Dazu 11 neue Ausdrücke mit Auslösern.

## Layout (Sprite 320×200, Statusstreifen darunter unverändert)
Augenmitten x = 90 / 230, y = 96; Auge 64 × 72 (Basis), Nase (160, 146) r 4; Mund-Mitte y 172. Gaze: Pupillen wandern innerhalb des Auges (±14 px x, ±10 px y), das Gesicht selbst nur ±8 / ±4 px.

## Gesichtsmodell (`src/logic/face_model.h`, testbar)
```
enum class Mouth : uint8_t { CURVE, O, ZIG, WAVE, KISS, TONGUE };
struct FaceShape {
  EyeShape eye;            // w, h, radius, open, lid_top_tilt, lid_top_cut, lid_bottom_cut, gaze_x, gaze_y (wie bisher)
  float pupil_r, pupil_dx, pupil_dy;
  float brow_y, brow_tilt, brow_curve, brow_asym;   // y relativ zur Augenoberkante (negativ = höher)
  float mouth_curve, mouth_open, mouth_w;
  Mouth mouth; bool wink;  // wink: rechtes Auge zu
};
struct FaceExpr { FaceShape s; Rgb color; };
FaceExpr face_expression(Emotion e);                 // Tabelle aus dem Mockup (E-Liste), alle Emotion-Werte
FaceShape face_tween(const FaceShape &cur, const FaceShape &target, uint32_t dt_ms);   // floats wie eye_tween; mouth/wink springen, sobald k-Summe ≥ 0,5 (Hälfte des Übergangs)
```
Umrechnung Mockup → Modell: `open, tilt→lid_top_tilt, topCut→lid_top_cut, botCut→lid_bottom_cut, pr→pupil_r, pdx/pdy→pupil_dx/dy, by→brow_y, bt→brow_tilt, bc→brow_curve, bAsym→brow_asym, mc→mouth_curve, mo→mouth_open, mw→mouth_w, ms→mouth (line=CURVE, o=O, zig=ZIG, wave=WAVE, kiss=KISS; „frech (Zunge)“ = TONGUE), wink`. Feste Werte: w 64, h 72, radius 20. Farben wie im Mockup (Konstanten RED, PINK, …). Extras (♥, ?, ♪, zZ, Ärger) sind Overlays, nicht Teil der Form.

## Neue Emotionen (in `Emotion` vor `COUNT`)
`CONFUSED, MISCHIEVOUS, SCARED, SKEPTICAL, PROUD, GIGGLE, YAWN, WHISTLE, KISS, CHEEKY, STARTLED` (= verwirrt, schelmisch, ängstlich, skeptisch, stolz, kichert, gähnt, pfeift, Kussmund, frech, erschrocken).

## Auslöser (`src/logic/mood.h`, testbar)
| Emotion | Auslöser | Dauer |
|---|---|---|
| GIGGLE | 3. Streicheln (gezählt, auch innerhalb des Cooldowns) innerhalb 20 s | 3 s |
| KISS | Streicheln, wenn Zuneigung danach > 90 (statt LOVE) | 3 s |
| CONFUSED | TAP < 500 ms nach dem vorigen TAP (statt SURPRISED) | 2,5 s |
| SCARED | POKE, während Grundgefühl SAD (statt ANNOYED; Zuneigung −3 bleibt) | 5 s |
| PROUD | nach GAME_WON: EXCITED 6 s, danach PROUD 4 s (Folge-Gefühl) | 4 s |
| SKEPTICAL | GAME_LOST | 4 s |
| STARTLED | Gewitter-Overlay alle 4 min für 1 s (ersetzt bisher SURPRISED-Form in app.cpp) | 1 s |
| YAWN | „Eigenart“: Grundgefühl SLEEPY, alle 2–5 min (zufällig) | 3 s |
| WHISTLE | Eigenart: Grundgefühl BORED, alle 1–3 min | 5 s |
| MISCHIEVOUS / CHEEKY | Eigenart: Grundgefühl HAPPY, alle 2–6 min, abwechselnd zufällig | 3 s |

Vorrang beim Streicheln: GIGGLE (3. in 20 s) vor KISS vor LOVE. `eye_expression(e)` bleibt für Spiele/Uhr erhalten und liefert für alle Emotionen `face_expression(e).s.eye` + Farbe (der bestehende Test `test_table_complete` läuft über `COUNT`).

Eigenarten: `void mood_quirks(MoodState &s, int hour, uint32_t now_ms)` jedes Bild; eigener Zufallszustand im `MoodState` (Seed per `mood_seed(s, seed)`), nächster Zeitpunkt wird beim Wechsel des Grundgefühls neu gezogen; Eigenarten nur, wenn kein anderes kurzzeitiges Gefühl läuft und er nicht schläft. „Grundgefühl“ = `mood_emotion` ohne kurzzeitige Gefühle (neue Hilfsfunktion `mood_base_emotion`).

## Overlays (`weather_mood.h`)
Bestehend bleibt. Neu: CONFUSED → Fragezeichen (`OV_QUESTION`), WHISTLE → Note (`OV_NOTE`), KISS → kleines Herz (`OV_HEART`), YAWN/SLEEPING → zZ nur bei SLEEPING. Sonnenbrille zusätzlich bei PROUD/MISCHIEVOUS/CHEEKY (sonnig, Tag).

## Zeichnen (`src/ui/eyes.*` → Gesicht)
`eyes_draw(lcd, const FaceShape&, Rgb, overlays, now)` zeichnet in das bestehende 8-Bit-Sprite nach der Mockup-Funktion `face()`: Auge (abgerundetes Rechteck), Pupille (dunkel `#001820`-artig: Farbe × 0,1) + weißer Glanzpunkt, schwarzes oberes Lid (Neigung), schwarzes unteres Lid (Ellipse), Braue (Linie mit Bogen, Dicke 7, aus `fillTriangle`/`drawWideLine`), Nase, Mund je Form (Linie/gefüllt bei `mouth_open>0`, O-Ring, Zickzack, Welle, Kuss „3“, Zunge = CURVE + rosa Halbkreis). `eyes_draw_pair()` für Spiele/Uhr bleibt unverändert (nur Augen).
Blinzeln multipliziert `eye.open`; beim Blinzeln verschwinden die Pupillen (wie Mockup: nur wenn Augenhöhe > 20 px). Gazer-Ziel → `pupil_dx/dy` (Grenzen s. Layout) und kleiner Gesichts-Versatz.

## Prüfung
- `pio test -e native`: neue Gruppe `test_face_model` (Tabelle vollständig für alle Emotionen inkl. neuer, Werte-Stichproben aus dem Mockup, Tween konvergiert, Mund springt erst in der zweiten Hälfte), `test_mood` erweitert (jeder Auslöser der Tabelle + Eigenarten mit festem Seed + keine Eigenart beim Schlafen). Gegenprobe je Gruppe.
- `pio run -e cyd2usb` ohne Fehler, fps ≥ 25 (seriell), Heap-Minimum im Rahmen.
- Am Gerät (Nutzer): Gesicht sieht aus wie der Mockup, Ausdrücke wechseln weich.
