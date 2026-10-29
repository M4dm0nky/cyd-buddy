# Kamera-Firmware Phase A (XIAO) – Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Der XIAO ESP32S3 Sense erkennt Gesichter, meldet sie im Zeilenprotokoll über UART und kann sich per WLAN selbst aktualisieren. Er wird per USB-C geflasht und am Gerät vermessen, **bevor** er verlötet und eingebaut wird.

**Architecture:** Neues PlatformIO-Projekt `cam/` neben dem CYD-Projekt. Protokoll und reine Entscheidungslogik liegen als Header in `src/logic/` (`cam_proto.h`, `cam_xiao.h`). Beide Projekte nutzen sie, nativ getestet. `ota_policy.h` bekommt einen Chip-Parameter, damit der XIAO dieselbe Image-Prüfung nutzt. Die XIAO-Firmware besteht aus dünnen Modulen: `camera`, `detect`, `link`, `updater`, `main`.

**Tech Stack:** PlatformIO (pioarduino), Arduino-ESP32, esp32-camera, ESP-DL (Gesichtserkennung), Unity (native Tests), Python + pyserial (serielles Hilfsskript).

Spec: `docs/superpowers/specs/2026-10-26-kamera-firmware-design.md`. Phase B (CYD: `cam_link`, `presence`, UI) und Phase C (`ota-publish` mit cam-Image, Update-Karte am CYD) bekommen eigene Pläne.

## Global Constraints

- Warum Phase A zuerst: Der USB-C des XIAO ist nach dem Einbau unerreichbar. Was nur per Kabel geht, muss **in diesem ersten Flash** stecken: **Partitionstabelle mit zwei App-Slots (`default_8MB.csv`)**, Bootloader mit Rollback, **WLAN-Update über die `U`-Zeile** und Rollback nach einem gescheiterten Probelauf.
- UART: 115200 Baud, 8N1. XIAO: D6 = GPIO43 = TX, D7 = GPIO44 = RX. CYD (Phase B): `Serial2`, RX = GPIO 27, TX = GPIO 22.
- Zeile: `<inhalt>*<xx>\n`, `xx` = XOR über `<inhalt>` als zwei Hex-Ziffern (gesendet groß, gelesen in beiden Schreibweisen). Bei falscher Prüfsumme, unbekanntem Befehl oder Überlänge wird die Zeile verworfen.
- **Abweichung von der Spec:** Die Höchstlänge ist **400** Zeichen statt 200 (`CAM_LINE_MAX`). Die `U`-Zeile mit URL, SHA-256, SSID und Passwort passt nicht in 200. Die Spec wird in Task 2 entsprechend korrigiert.
- Zeilen: `F <x> <y> <w>` (je 0–1000), `N`, `H <version> on|off|err`, `U <prozent>`, `U ok`, `U err <grund>`, `C 1`/`C 0`, `U⇥<url>⇥<sha256>⇥<ssid>⇥<passwort>` (⇥ = Tab).
- Lebenszeichen alle 2 s. Probelauf nach dem Update: bestätigt nach ≥ 20 s ohne Kamerafehler und ≥ 5 Lebenszeichen, sonst Rollback spätestens nach 60 s.
- WLAN nur während eines Updates. Die Zugangsdaten werden nicht gespeichert. Bilder verlassen den XIAO nie.
- Download-URLs nur über `ota_redirect_ok()` (https, github.com oder `*.githubusercontent.com`), in jedem Umleitungsschritt.
- Builds und Tests **nur über das PowerShell-Werkzeug**, nie aus Git Bash. pio: `C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe`. Für `pio test -e native` vorher `$env:PATH = "C:\Users\madma\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;" + $env:PATH`.
- Tests gezielt: `pio test -e native -f test_cam_proto` usw. Der volle Lauf (`pio test -e native`, `pio run -e cyd2usb`) kommt nur einmal in Task 5. Ausgaben auf Fehler und Summe filtern.
- Jeder neue Test wird einmal mit einem absichtlich eingebauten Fehler rot gesehen.
- Commits: Branch `feature/kamera-firmware`, ein Commit pro Task, **keine** `Co-Authored-By`-Zeile, kein Hinweis auf Claude.
- Jeder Task trägt einen Eintrag oben in `docs/FORTSCHRITT.md` ein (Format wie die vorhandenen: Zeit, Gemacht, Prüfung, Offen).
- Kein Prozess bleibt laufen: Serielle Mitschnitte nur mit Zeitlimit (`tools/cam-serial.py --seconds`). Nach Abbruch `Get-CimInstance Win32_Process` auf `python`/`pio`/`esptool` prüfen. Kein `python - <<EOF` in Git Bash.
- Code-Stil: wie `src/logic/*.h` (header-only, `static inline`, deutsche Kommentare, kurze Zeilenkommentare, keine Arduino-Includes in `src/logic/`).

## Dateien

| Datei | Aufgabe |
|---|---|
| `src/logic/cam_proto.h` (neu) | Prüfsumme, Zeilen bauen/zerlegen, `CamLineBuf` (Bytes → Zeilen). Von XIAO und CYD genutzt. |
| `src/logic/cam_xiao.h` (neu) | Box → `CamFace` (0–1000), Entscheidung im Probelauf (`cam_health`). |
| `src/logic/ota_policy.h` (ändern, Z. 14 und 25–28) | `OTA_CHIP_ESP32S3`, Chip-Parameter in `ota_image_check`. |
| `test/test_cam_proto/test_main.cpp` (neu) | Tests für beide neuen Header. |
| `test/test_ota_policy/test_main.cpp` (ändern) | Test für S3-Chip. |
| `cam/platformio.ini`, `cam/src/cam_config.h` (neu) | Projekt, Pins, Zeiten, Version. |
| `cam/src/camera.{h,cpp}` (neu) | Kamera an/aus. |
| `cam/src/detect.{h,cpp}` (neu) | Gesichtserkennung → größtes Gesicht. |
| `cam/src/link.{h,cpp}` (neu) | UART + USB-Spiegel, Zeilen lesen. |
| `cam/src/updater.{h,cpp}` (neu) | WLAN-Update, Probelauf, Rollback. |
| `cam/src/main.cpp` (neu) | Ablauf. |
| `tools/cam-serial.py` (neu) | Serieller Mitschnitt mit Zeitlimit, sendet Zeilen mit Prüfsumme. |

---

### Task 1: Spike – `cam/` baut mit Kamera und Gesichtserkennung, erster Flash

Ziel: Weg festlegen und die Rechenzeit pro Bild am echten XIAO messen. Ein Spike-Flash per USB ist unkritisch, denn bis zum Einbau kann beliebig oft neu geflasht werden.

**Files:**
- Create: `cam/platformio.ini`, `cam/src/cam_config.h`, `cam/src/camera.h`, `cam/src/camera.cpp`, `cam/src/detect.h`, `cam/src/detect.cpp`, `cam/src/main.cpp` (Spike-Fassung, wird in Task 4 ersetzt)

**Interfaces:**
- Produces: `bool camera_start()`, `void camera_stop()`, `bool camera_running()`; `bool detect_init()`, `bool detect_face(camera_fb_t *fb, CamFace *out, uint32_t *infer_ms)` (true = Gesicht gefunden, `out` = größtes). `CamFace` und `cam_box_to_face` legt der Spike gleich in `src/logic/cam_xiao.h` an, genau wie in Task 3 Step 3, nur ohne den `cam_health`-Teil. Task 3 ergänzt Tests und `cam_health`.

**Zwei Wege, in dieser Reihenfolge. Weg 1 hat ein festes Zeitlimit: höchstens zwei vollständige Build-Versuche. Scheitert der Build oder liegt die Rechenzeit über 250 ms/Bild, gilt Weg 2.**

- [ ] **Step 1: Branch anlegen**

```bash
git switch -c feature/kamera-firmware main
```

- [ ] **Step 2: `cam/src/cam_config.h` (gilt für beide Wege)**

```cpp
#pragma once
// XIAO ESP32S3 Sense: Kamera-Pins (Seeed-Wiki, OV2640/OV3660-Modul), UART zum CYD, Zeiten.
#define CAM_PIN_PWDN  -1
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK  10
#define CAM_PIN_SIOD  40
#define CAM_PIN_SIOC  39
#define CAM_PIN_D7    48
#define CAM_PIN_D6    11
#define CAM_PIN_D5    12
#define CAM_PIN_D4    14
#define CAM_PIN_D3    16
#define CAM_PIN_D2    18
#define CAM_PIN_D1    17
#define CAM_PIN_D0    15
#define CAM_PIN_VSYNC 38
#define CAM_PIN_HREF  47
#define CAM_PIN_PCLK  13

#define CAM_UART_BAUD 115200
#define CAM_UART_TX   43   // D6
#define CAM_UART_RX   44   // D7

#define CAM_HEART_MS  2000
#define CAM_RETRY_MS  10000   // Kamera-Init nach Fehler erneut versuchen

#ifndef CAM_FW_VERSION
#define CAM_FW_VERSION "0.1.0"   // Phase C setzt die Version beim Release
#endif
```

- [ ] **Step 3: `cam/src/camera.h` und `camera.cpp` (beide Wege)**

```cpp
// camera.h
#pragma once
bool camera_start();    // true = Sensor läuft (QVGA, RGB565)
void camera_stop();     // XCLK aus, Puffer frei (PWDN ist beim XIAO nicht verdrahtet)
bool camera_running();
```

```cpp
// camera.cpp
#include <esp_camera.h>
#include "camera.h"
#include "cam_config.h"

static bool g_on = false;

bool camera_start() {
#ifdef CAM_TEST_BROKEN
  return false;   // Testbild für den Rollback (Task 5)
#endif
  if (g_on) return true;
  camera_config_t c = {};
  c.pin_pwdn = CAM_PIN_PWDN;   c.pin_reset = CAM_PIN_RESET; c.pin_xclk = CAM_PIN_XCLK;
  c.pin_sccb_sda = CAM_PIN_SIOD; c.pin_sccb_scl = CAM_PIN_SIOC;
  c.pin_d7 = CAM_PIN_D7; c.pin_d6 = CAM_PIN_D6; c.pin_d5 = CAM_PIN_D5; c.pin_d4 = CAM_PIN_D4;
  c.pin_d3 = CAM_PIN_D3; c.pin_d2 = CAM_PIN_D2; c.pin_d1 = CAM_PIN_D1; c.pin_d0 = CAM_PIN_D0;
  c.pin_vsync = CAM_PIN_VSYNC; c.pin_href = CAM_PIN_HREF; c.pin_pclk = CAM_PIN_PCLK;
  c.xclk_freq_hz = 20000000;
  c.ledc_timer = LEDC_TIMER_0; c.ledc_channel = LEDC_CHANNEL_0;
  c.pixel_format = PIXFORMAT_RGB565;
  c.frame_size = FRAMESIZE_QVGA;
  c.fb_count = 2;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_LATEST;
  g_on = esp_camera_init(&c) == ESP_OK;
  return g_on;
}

void camera_stop() {
  if (!g_on) return;
  esp_camera_deinit();
  g_on = false;
}

bool camera_running() { return g_on; }
```

- [ ] **Step 4a: Weg 1 – `cam/platformio.ini` (pioarduino, Arduino als IDF-Komponente, ESP-DL v3)**

```ini
[env:cam]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/55.03.312-1/platform-espressif32.zip
board = seeed_xiao_esp32s3
framework = arduino, espidf
board_build.partitions = default_8MB.csv
board_build.arduino.memory_type = qio_opi
monitor_speed = 115200
build_flags =
    -I../src
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=1
```

Zusätzlich `cam/src/idf_component.yml`:

```yaml
dependencies:
  espressif/esp32-camera: "*"
  espressif/human_face_detect: "*"
```

und `cam/sdkconfig.defaults`:

```
CONFIG_FREERTOS_HZ=1000
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

Weil `CONFIG_PARTITION_TABLE_CUSTOM` eine Datei verlangt, `cam/partitions.csv` mit dem Inhalt von `default_8MB.csv` anlegen:

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x330000,
app1,     app,  ota_1,   0x340000,0x330000,
spiffs,   data, spiffs,  0x670000,0x180000,
coredump, data, coredump,0x7F0000,0x10000,
```

und in `platformio.ini` `board_build.partitions = partitions.csv` setzen.

`cam/src/detect.cpp` (Weg 1). Die Feldnamen von `dl::image::img_t` und der Name des Pixeltyps für **Big-Endian-RGB565** (die esp32-camera liefert RGB565 big-endian) stehen im heruntergeladenen Header `managed_components/espressif__human_face_detect/.../human_face_detect.hpp` bzw. `dl_image_define.hpp`. Dort nachsehen und genau diese Namen verwenden:

```cpp
#include <Arduino.h>
#include <esp_camera.h>
#include "human_face_detect.hpp"
#include "detect.h"
#include "logic/cam_xiao.h"

static HumanFaceDetect *g_det = nullptr;

bool detect_init() {
  g_det = new HumanFaceDetect();
  return g_det != nullptr;
}

bool detect_face(camera_fb_t *fb, CamFace *out, uint32_t *infer_ms) {
  const uint32_t t0 = millis();
  dl::image::img_t img = {};
  img.data = fb->buf;
  img.width = fb->width;
  img.height = fb->height;
  img.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565;   // Big-Endian-Variante laut Header wählen
  std::list<dl::detect::result_t> &res = g_det->run(img);
  *infer_ms = millis() - t0;
  bool found = false;
  int best = 0;
  for (auto &r : res) {
    CamFace f;
    if (!cam_box_to_face(r.box[0], r.box[1], r.box[2], r.box[3], fb->width, fb->height, &f)) continue;
    if (!found || f.w > best) { *out = f; best = f.w; found = true; }
  }
  return found;
}
```

- [ ] **Step 4b: Weg 2 (Fallback) – Arduino-Kern 2.0.17 mit eingebauter ESP-DL (MSR01 + MNP01)**

`cam/platformio.ini`:

```ini
[env:cam]
platform = espressif32@6.9.0
board = seeed_xiao_esp32s3
framework = arduino
board_build.partitions = default_8MB.csv
board_build.arduino.memory_type = qio_opi
monitor_speed = 115200
build_flags =
    -I../src
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=1
```

`cam/src/detect.cpp` (Weg 2, entspricht dem Beispiel `CameraWebServer` im Kern 2.0.x):

```cpp
#include <Arduino.h>
#include <esp_camera.h>
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "detect.h"
#include "logic/cam_xiao.h"

static HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
static HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);

bool detect_init() { return true; }

bool detect_face(camera_fb_t *fb, CamFace *out, uint32_t *infer_ms) {
  const uint32_t t0 = millis();
  std::list<dl::detect::result_t> &cand = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
  std::list<dl::detect::result_t> &res = s2.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3}, cand);
  *infer_ms = millis() - t0;
  bool found = false;
  int best = 0;
  for (auto &r : res) {
    CamFace f;
    if (!cam_box_to_face(r.box[0], r.box[1], r.box[2], r.box[3], fb->width, fb->height, &f)) continue;
    if (!found || f.w > best) { *out = f; best = f.w; found = true; }
  }
  return found;
}
```

Bei beiden Wegen ist `detect.h`:

```cpp
#pragma once
#include <stdint.h>
#include <esp_camera.h>
#include "logic/cam_xiao.h"
bool detect_init();
bool detect_face(camera_fb_t *fb, CamFace *out, uint32_t *infer_ms);   // true = Gesicht, out = größtes
```

`src/logic/cam_xiao.h` im Spike: der Code aus Task 3 Step 3 bis einschließlich `cam_box_to_face`, ohne den `cam_health`-Teil.

- [ ] **Step 5: Spike-`main.cpp` (misst nur, sendet noch nichts zum CYD)**

```cpp
#include <Arduino.h>
#include <esp_camera.h>
#include "camera.h"
#include "detect.h"

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.printf("# psram=%u\n", (unsigned)ESP.getPsramSize());
  Serial.printf("# camera=%d detect=%d\n", camera_start(), detect_init());
}

void loop() {
  static uint32_t t0 = millis(), frames = 0, ms_sum = 0;
  static CamFace last = {0, 0, 0};
  static bool has = false;
  if (!camera_running()) { delay(500); return; }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;
  uint32_t ms = 0;
  CamFace f;
  has = detect_face(fb, &f, &ms);
  if (has) last = f;
  esp_camera_fb_return(fb);
  frames++; ms_sum += ms;
  if (millis() - t0 >= 1000) {
    Serial.printf("# fps=%u ms=%u face=%d %d,%d,%d\n", (unsigned)frames, (unsigned)(frames ? ms_sum / frames : 0), has, last.x, last.y, last.w);
    t0 = millis(); frames = 0; ms_sum = 0;
  }
}
```

- [ ] **Step 6: Bauen (PowerShell)**

```powershell
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" run -d cam 2>&1 | Select-String 'error|SUCCESS|FAILED|RAM:|Flash:' | Select-Object -Last 20
```

Expected: `SUCCESS`. Bei Weg 1 und zwei gescheiterten Versuchen: `cam/` auf Weg 2 umstellen (Step 4b) und erneut bauen. **Bericht des Subagenten: welcher Weg, Build-Ausgabe (gefiltert), Flash-Größe.**

- [ ] **Step 7: `tools/cam-serial.py` anlegen** (wird in Task 1, 4 und 5 gebraucht)

```python
"""Serieller Mitschnitt mit festem Zeitlimit. Sendet optional Zeilen mit Prüfsumme (Protokoll src/logic/cam_proto.h).
Aufruf: python tools/cam-serial.py COM7 --seconds 20 [--send "C 0"] [--send-at 5]
"""
import argparse
import sys
import time

import serial


def seal(content: str) -> bytes:
    x = 0
    for b in content.encode("utf-8"):
        x ^= b
    return f"{content}*{x:02X}\n".encode("utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("port")
    ap.add_argument("--seconds", type=float, default=20)
    ap.add_argument("--send", action="append", default=[], help="Inhalt ohne Prüfsumme; \\t wird zu Tab")
    ap.add_argument("--send-at", type=float, default=3, help="Sekunden nach Start, dann alle 2 s die nächste")
    a = ap.parse_args()
    with serial.Serial(a.port, 115200, timeout=0.2) as s:
        start = time.time()
        pending = [c.replace("\\t", "\t") for c in a.send]
        next_send = start + a.send_at
        while time.time() - start < a.seconds:
            if pending and time.time() >= next_send:
                s.write(seal(pending.pop(0)))
                next_send = time.time() + 2
            line = s.readline()
            if line:
                sys.stdout.write(f"{time.time() - start:6.1f} {line.decode('utf-8', 'replace').rstrip()}\n")
                sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

Prüfen, ob pyserial für den Python-Interpreter da ist:

```powershell
& "C:\Users\madma\AppData\Local\Python\pythoncore-3.14-64\python.exe" -c "import serial; print(serial.__version__)"
```

Fehlt es: `& "C:\Users\madma\AppData\Local\Python\pythoncore-3.14-64\python.exe" -m pip install --user pyserial`.

- [ ] **Step 8: Erster Flash – NUTZER-SCHRITT, vorher ansagen (Opus, nicht der Subagent)**

Ansage an den Nutzer: „Bitte den XIAO per USB-C an den PC stecken. Die Kamera muss am Board aufgesteckt sein. Klappt der Upload nicht: Taste **B** (BOOT) gedrückt halten, USB-C ein- und wieder ausstecken bzw. **R** (Reset) drücken, B loslassen. Nach dem Upload einmal **R** drücken.“

Port finden und flashen:

```powershell
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" device list | Select-String 'COM|303A'
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" run -d cam -t upload --upload-port COMx 2>&1 | Select-String 'error|SUCCESS|FAILED|Wrote|Hash' | Select-Object -Last 10
```

Expected: `Hash of data verified` und `SUCCESS`. Beim allerersten Flash schreibt pio Bootloader, Partitionstabelle, `boot_app0` und App. Genau so soll es sein.

- [ ] **Step 9: Messen – NUTZER-SCHRITT**

Ansage: „Setz dich etwa 60 cm vor die Kamera und schau hinein, 20 Sekunden lang.“

```powershell
& "C:\Users\madma\AppData\Local\Python\pythoncore-3.14-64\python.exe" tools/cam-serial.py COMx --seconds 25
```

Expected: `# psram=8388608` (≈ 8 MB), `camera=1 detect=1`, Zeilen `# fps=… ms=… face=1 …`. Entscheidung: Weg 1 bleibt nur bei `ms` ≤ 250 und sicherer Erkennung, sonst Weg 2 (Step 4b, neu bauen, flashen, messen).

- [ ] **Step 10: Commit**

```bash
git add cam/ src/logic/cam_xiao.h tools/cam-serial.py docs/FORTSCHRITT.md
git commit -m "feat(cam): Spike XIAO – Kamera und Gesichtserkennung, Messung am Geraet"
```

FORTSCHRITT-Eintrag: gewählter Weg, Grund, fps, ms/Bild, Flash-Größe.

---

### Task 2: Zeilenprotokoll `src/logic/cam_proto.h`

**Files:**
- Create: `src/logic/cam_proto.h`, `test/test_cam_proto/test_main.cpp`
- Modify: `docs/superpowers/specs/2026-10-26-kamera-firmware-design.md` (Zeile 24: „mehr als 200 Zeichen“ → „mehr als 400 Zeichen (die `U`-Zeile braucht mehr als 200)“)

**Interfaces:**
- Produces (exakt so, Phase B nutzt es am CYD):

```cpp
constexpr size_t CAM_LINE_MAX = 400;
enum CamKind : uint8_t { CAM_NONE = 0, CAM_FACE, CAM_NOFACE, CAM_HEART, CAM_CTRL, CAM_UPD_CMD, CAM_UPD_PCT, CAM_UPD_OK, CAM_UPD_ERR };
enum CamState : uint8_t { CAM_ST_ON = 0, CAM_ST_OFF, CAM_ST_ERR };
struct CamMsg { CamKind kind; int16_t x, y, w; char version[16]; CamState state; bool on; uint8_t pct; char text[40]; char url[200], sha[65], ssid[33], pass[64]; };
uint8_t cam_xor(const char *s, size_t n);
size_t cam_seal(const char *content, char *out, size_t cap);           // "content*XX\n", Länge oder 0
size_t cam_build_face(int x, int y, int w, char *out, size_t cap);
size_t cam_build_noface(char *out, size_t cap);
size_t cam_build_heart(const char *version, CamState st, char *out, size_t cap);
size_t cam_build_ctrl(bool on, char *out, size_t cap);
size_t cam_build_upd_pct(int pct, char *out, size_t cap);
size_t cam_build_upd_ok(char *out, size_t cap);
size_t cam_build_upd_err(const char *why, char *out, size_t cap);
size_t cam_build_upd_cmd(const char *url, const char *sha, const char *ssid, const char *pass, char *out, size_t cap);
bool cam_parse(const char *line, size_t n, CamMsg *m);                // line ohne '\n'
struct CamLineBuf { bool feed(char c); const char *line() const; size_t len() const; };
```

- [ ] **Step 1: Failing test schreiben – `test/test_cam_proto/test_main.cpp`**

```cpp
#include <unity.h>
#include <string.h>
#include "logic/cam_proto.h"
void setUp() {}
void tearDown() {}

static char g_buf[CAM_LINE_MAX + 8];
// Inhalt mit gültiger Prüfsumme, ohne '\n' (wie cam_parse sie bekommt)
static const char *seal(const char *content) {
  size_t n = cam_seal(content, g_buf, sizeof g_buf);
  TEST_ASSERT_TRUE(n > 0);
  g_buf[n - 1] = '\0';
  return g_buf;
}
static bool parse_str(const char *s, CamMsg *m) { return cam_parse(s, strlen(s), m); }

void test_xor_known() {
  TEST_ASSERT_EQUAL_HEX8(0x4E, cam_xor("N", 1));
  TEST_ASSERT_EQUAL_HEX8(0x53, cam_xor("C 0", 3));
  TEST_ASSERT_EQUAL_HEX8(0x52, cam_xor("C 1", 3));
}
void test_build_fixed_lines() {
  char b[32];
  TEST_ASSERT_EQUAL(5, cam_build_noface(b, sizeof b));
  TEST_ASSERT_EQUAL_STRING("N*4E\n", b);
  TEST_ASSERT_EQUAL(7, cam_build_ctrl(true, b, sizeof b));
  TEST_ASSERT_EQUAL_STRING("C 1*52\n", b);
  cam_build_ctrl(false, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("C 0*53\n", b);
  TEST_ASSERT_EQUAL(0, cam_build_noface(b, 4));   // Puffer zu klein
}
void test_roundtrip_face_and_heart() {
  char b[64];
  CamMsg m;
  size_t n = cam_build_face(512, 430, 210, b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_FACE, m.kind);
  TEST_ASSERT_EQUAL(512, m.x); TEST_ASSERT_EQUAL(430, m.y); TEST_ASSERT_EQUAL(210, m.w);
  n = cam_build_heart("0.1.0", CAM_ST_ERR, b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_HEART, m.kind);
  TEST_ASSERT_EQUAL_STRING("0.1.0", m.version);
  TEST_ASSERT_EQUAL(CAM_ST_ERR, m.state);
  TEST_ASSERT_TRUE(parse_str("N*4E", &m));
  TEST_ASSERT_EQUAL(CAM_NOFACE, m.kind);
  TEST_ASSERT_TRUE(parse_str("C 0*53", &m));
  TEST_ASSERT_EQUAL(CAM_CTRL, m.kind);
  TEST_ASSERT_FALSE(m.on);
  TEST_ASSERT_TRUE(parse_str("C 1*52\r", &m));   // '\r' am Ende wird ignoriert
  TEST_ASSERT_TRUE(m.on);
}
void test_checksum_rules() {
  CamMsg m;
  TEST_ASSERT_FALSE(parse_str("N*4F", &m));
  TEST_ASSERT_TRUE(parse_str("C 0*53", &m));
  char b[64];
  size_t n = cam_build_face(1, 2, 3, b, sizeof b);
  for (size_t i = n - 3; i < n - 1; i++) if (b[i] >= 'A' && b[i] <= 'F') b[i] = (char)(b[i] - 'A' + 'a');
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));      // Kleinbuchstaben gelten auch
}
void test_rejects_garbage() {
  CamMsg m;
  const char *raw[] = {"", "*", "N", "N*4", "N*4EE", "N*G1"};
  for (const char *s : raw) TEST_ASSERT_FALSE_MESSAGE(parse_str(s, &m), s);
  const char *bad[] = {"X", "F 1 2", "F 1 2 3 4", "F 1001 0 0", "F -1 0 0", "F 1a 2 3", "F  1 2 3",
                       "H 0.1.0 maybe", "H bad/ver on", "H 0.1.0", "H 12345678901234567 on", "C 2", "C",
                       "U 101", "U -1", "U", "N x", "U err "};
  for (const char *s : bad) TEST_ASSERT_FALSE_MESSAGE(parse_str(seal(s), &m), s);
}
void test_length_limit() {
  char content[CAM_LINE_MAX + 2];
  memset(content, 'a', sizeof content);
  content[CAM_LINE_MAX - 3] = '\0';               // passt genau: Inhalt + "*XX" = CAM_LINE_MAX
  char out[CAM_LINE_MAX + 8];
  TEST_ASSERT_EQUAL(CAM_LINE_MAX + 1, cam_seal(content, out, sizeof out));
  content[CAM_LINE_MAX - 3] = 'a'; content[CAM_LINE_MAX - 2] = '\0';
  TEST_ASSERT_EQUAL(0, cam_seal(content, out, sizeof out));
  CamMsg m;
  TEST_ASSERT_FALSE(cam_parse(out, CAM_LINE_MAX + 1, &m));
  TEST_ASSERT_EQUAL(0, cam_seal("a\nb", out, sizeof out));   // Zeilenumbruch im Inhalt verboten
}
void test_update_cmd_roundtrip() {
  char sha[65]; memset(sha, 'a', 64); sha[64] = '\0';
  const char *url = "https://github.com/M4dm0nky/cyd-buddy/releases/download/v20261029-1200/cam.bin";
  char b[CAM_LINE_MAX + 8];
  size_t n = cam_build_upd_cmd(url, sha, "Mein WLAN", "p*ss w0rd", b, sizeof b);
  TEST_ASSERT_TRUE(n > 0);
  CamMsg m;
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_CMD, m.kind);
  TEST_ASSERT_EQUAL_STRING(url, m.url);
  TEST_ASSERT_EQUAL_STRING(sha, m.sha);
  TEST_ASSERT_EQUAL_STRING("Mein WLAN", m.ssid);
  TEST_ASSERT_EQUAL_STRING("p*ss w0rd", m.pass);
  n = cam_build_upd_cmd(url, sha, "offen", "", b, sizeof b);    // offenes WLAN: leeres Passwort erlaubt
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL_STRING("", m.pass);
}
void test_update_cmd_rejects() {
  char sha[65]; memset(sha, 'a', 64); sha[64] = '\0';
  char ssid33[34]; memset(ssid33, 's', 33); ssid33[33] = '\0';
  CamMsg m;
  char line[CAM_LINE_MAX];
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%.63s\tW\tp", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SHA zu kurz
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\tW", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // nur 4 Felder
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\t\tp", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SSID leer
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\t%s\tp", sha, ssid33);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SSID zu lang
  sha[3] = 'G';
  snprintf(line, sizeof line, "U\thttps://github.com/x\t%s\tW\tp", sha);
  TEST_ASSERT_FALSE(parse_str(seal(line), &m));   // SHA kein Hex
}
void test_update_progress() {
  char b[64];
  CamMsg m;
  size_t n = cam_build_upd_pct(42, b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_PCT, m.kind); TEST_ASSERT_EQUAL(42, m.pct);
  n = cam_build_upd_ok(b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_OK, m.kind);
  n = cam_build_upd_err("SHA-256 falsch", b, sizeof b);
  TEST_ASSERT_TRUE(cam_parse(b, n - 1, &m));
  TEST_ASSERT_EQUAL(CAM_UPD_ERR, m.kind); TEST_ASSERT_EQUAL_STRING("SHA-256 falsch", m.text);
}
void test_linebuf_split_and_cr() {
  CamLineBuf lb;
  const char *in = "N*4";
  for (const char *p = in; *p; p++) TEST_ASSERT_FALSE(lb.feed(*p));
  TEST_ASSERT_TRUE(lb.feed('E') == false);
  TEST_ASSERT_TRUE(lb.feed('\n'));
  TEST_ASSERT_EQUAL_STRING("N*4E", lb.line());
  TEST_ASSERT_EQUAL(4, lb.len());
  const char *in2 = "C 1*52\r\n";
  int done = 0;
  for (const char *p = in2; *p; p++) if (lb.feed(*p)) done++;
  TEST_ASSERT_EQUAL(1, done);
  TEST_ASSERT_EQUAL_STRING("C 1*52", lb.line());
  TEST_ASSERT_FALSE(lb.feed('\n'));               // leere Zeile zählt nicht
}
void test_linebuf_overflow_recovers() {
  CamLineBuf lb;
  for (int i = 0; i < 500; i++) TEST_ASSERT_FALSE(lb.feed('a'));
  TEST_ASSERT_FALSE(lb.feed('\n'));               // übergroße Zeile wird verworfen
  const char *in = "N*4E\n";
  bool ok = false;
  for (const char *p = in; *p; p++) ok = lb.feed(*p);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("N*4E", lb.line());
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_xor_known);
  RUN_TEST(test_build_fixed_lines);
  RUN_TEST(test_roundtrip_face_and_heart);
  RUN_TEST(test_checksum_rules);
  RUN_TEST(test_rejects_garbage);
  RUN_TEST(test_length_limit);
  RUN_TEST(test_update_cmd_roundtrip);
  RUN_TEST(test_update_cmd_rejects);
  RUN_TEST(test_update_progress);
  RUN_TEST(test_linebuf_split_and_cr);
  RUN_TEST(test_linebuf_overflow_recovers);
  return UNITY_END();
}
```

- [ ] **Step 2: Test rot sehen**

```powershell
$env:PATH = "C:\Users\madma\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;" + $env:PATH
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" test -e native -f test_cam_proto 2>&1 | Select-String 'error|FAIL|PASSED|ERRORED|Tests' | Select-Object -Last 15
```

Expected: ERRORED (`logic/cam_proto.h` fehlt).

- [ ] **Step 3: `src/logic/cam_proto.h` schreiben**

```cpp
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

// Zeilenprotokoll XIAO <-> CYD (Spec docs/superpowers/specs/2026-10-26-kamera-firmware-design.md). Reine Logik ohne
// Arduino-/IDF-Includes, von beiden Projekten benutzt (cam/ per -I../src). Host-Test test/test_cam_proto.
// Zeile: <inhalt>*<xx>\n, xx = XOR über <inhalt> als zwei Hexziffern (gesendet groß, gelesen beides).
constexpr size_t CAM_LINE_MAX = 400;   // ohne '\n'; die U-Zeile (URL, SHA, SSID, Passwort) braucht mehr als 200

enum CamKind : uint8_t { CAM_NONE = 0, CAM_FACE, CAM_NOFACE, CAM_HEART, CAM_CTRL, CAM_UPD_CMD, CAM_UPD_PCT, CAM_UPD_OK, CAM_UPD_ERR };
enum CamState : uint8_t { CAM_ST_ON = 0, CAM_ST_OFF, CAM_ST_ERR };

struct CamMsg {
  CamKind kind = CAM_NONE;
  int16_t x = 0, y = 0, w = 0;     // FACE, je 0..1000
  char version[16] = "";           // HEART
  CamState state = CAM_ST_OFF;     // HEART
  bool on = false;                 // CTRL
  uint8_t pct = 0;                 // UPD_PCT
  char text[40] = "";              // UPD_ERR: Grund
  char url[200] = "", sha[65] = "", ssid[33] = "", pass[64] = "";   // UPD_CMD
};

static inline uint8_t cam_xor(const char *s, size_t n) {
  uint8_t x = 0;
  for (size_t i = 0; i < n; i++) x ^= (uint8_t)s[i];
  return x;
}

// "content*XX\n" nach out. Rückgabe: Länge mit '\n' (ohne '\0'), 0 bei Überlänge, zu kleinem Puffer oder Zeilenumbruch im Inhalt.
static inline size_t cam_seal(const char *content, char *out, size_t cap) {
  const size_t n = strlen(content);
  if (n + 3 > CAM_LINE_MAX || n + 5 > cap) return 0;
  for (size_t i = 0; i < n; i++) if (content[i] == '\n' || content[i] == '\r') return 0;
  static const char kHex[] = "0123456789ABCDEF";
  const uint8_t x = cam_xor(content, n);
  memcpy(out, content, n);
  out[n] = '*';
  out[n + 1] = kHex[x >> 4];
  out[n + 2] = kHex[x & 15];
  out[n + 3] = '\n';
  out[n + 4] = '\0';
  return n + 4;
}

static inline size_t cam_build_face(int x, int y, int w, char *out, size_t cap) {
  char c[24]; snprintf(c, sizeof c, "F %d %d %d", x, y, w); return cam_seal(c, out, cap);
}
static inline size_t cam_build_noface(char *out, size_t cap) { return cam_seal("N", out, cap); }
static inline const char *cam_state_name(CamState s) { return s == CAM_ST_ON ? "on" : s == CAM_ST_ERR ? "err" : "off"; }
static inline size_t cam_build_heart(const char *version, CamState st, char *out, size_t cap) {
  char c[40]; snprintf(c, sizeof c, "H %.15s %s", version, cam_state_name(st)); return cam_seal(c, out, cap);
}
static inline size_t cam_build_ctrl(bool on, char *out, size_t cap) { return cam_seal(on ? "C 1" : "C 0", out, cap); }
static inline size_t cam_build_upd_pct(int pct, char *out, size_t cap) {
  char c[12]; snprintf(c, sizeof c, "U %d", pct); return cam_seal(c, out, cap);
}
static inline size_t cam_build_upd_ok(char *out, size_t cap) { return cam_seal("U ok", out, cap); }
static inline size_t cam_build_upd_err(const char *why, char *out, size_t cap) {
  char c[48]; snprintf(c, sizeof c, "U err %.39s", why); return cam_seal(c, out, cap);
}
static inline size_t cam_build_upd_cmd(const char *url, const char *sha, const char *ssid, const char *pass, char *out, size_t cap) {
  char c[CAM_LINE_MAX + 1];
  const int n = snprintf(c, sizeof c, "U\t%s\t%s\t%s\t%s", url, sha, ssid, pass);
  if (n < 0 || (size_t)n >= sizeof c) return 0;
  return cam_seal(c, out, cap);
}

// Ganzzahl aus genau den Zeichen [s, e), nur Ziffern, 0..maxv
static inline bool cam_num(const char *s, const char *e, int maxv, int *v) {
  if (s >= e || e - s > 5) return false;
  int r = 0;
  for (const char *p = s; p < e; p++) {
    if (*p < '0' || *p > '9') return false;
    r = r * 10 + (*p - '0');
  }
  if (r > maxv) return false;
  *v = r;
  return true;
}
static inline int cam_hexval(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
// Feld [s, e) nach out kopieren, wenn min..max Zeichen lang
static inline bool cam_field(const char *s, const char *e, size_t min, size_t max, char *out) {
  const size_t n = (size_t)(e - s);
  if (n < min || n > max) return false;
  memcpy(out, s, n);
  out[n] = '\0';
  return true;
}

static inline bool cam_parse_upd_cmd(const char *c, size_t n, CamMsg *m) {
  const char *f[5];
  const char *fe[5];
  const char *p = c + 2, *end = c + n;   // nach "U\t"
  for (int i = 0; i < 5; i++) {
    f[i] = p;
    while (p < end && *p != '\t') p++;
    fe[i] = p;
    if (i < 4) { if (p >= end) return false; p++; }
  }
  if (p != end) return false;   // mehr als 5 Felder
  if (!cam_field(f[0], fe[0], 9, sizeof m->url - 1, m->url)) return false;
  if (!cam_field(f[1], fe[1], 64, 64, m->sha)) return false;
  for (int i = 0; i < 64; i++) if (!((m->sha[i] >= '0' && m->sha[i] <= '9') || (m->sha[i] >= 'a' && m->sha[i] <= 'f'))) return false;
  if (!cam_field(f[2], fe[2], 1, 32, m->ssid)) return false;
  if (!cam_field(f[3], fe[3], 0, 63, m->pass)) return false;
  m->kind = CAM_UPD_CMD;
  return true;
}

// line ohne '\n' (ein '\r' am Ende wird ignoriert). true = gültige, bekannte Zeile; m ist dann gefüllt.
static inline bool cam_parse(const char *line, size_t n, CamMsg *m) {
  if (n > 0 && line[n - 1] == '\r') n--;
  if (n < 4 || n > CAM_LINE_MAX || line[n - 3] != '*') return false;
  const int h = cam_hexval(line[n - 2]), l = cam_hexval(line[n - 1]);
  if (h < 0 || l < 0) return false;
  const size_t cn = n - 3;
  if (cam_xor(line, cn) != (uint8_t)(h * 16 + l)) return false;
  const char *c = line, *end = line + cn;
  *m = CamMsg();
  if (cn == 1 && c[0] == 'N') { m->kind = CAM_NOFACE; return true; }
  if (cn >= 2 && c[0] == 'U' && c[1] == '\t') return cam_parse_upd_cmd(c, cn, m);
  if (cn < 3 || c[1] != ' ') return false;
  const char *a = c + 2;
  switch (c[0]) {
    case 'F': {
      const char *s1 = (const char *)memchr(a, ' ', end - a);
      if (!s1) return false;
      const char *s2 = (const char *)memchr(s1 + 1, ' ', end - s1 - 1);
      if (!s2 || memchr(s2 + 1, ' ', end - s2 - 1)) return false;
      int x, y, w;
      if (!cam_num(a, s1, 1000, &x) || !cam_num(s1 + 1, s2, 1000, &y) || !cam_num(s2 + 1, end, 1000, &w)) return false;
      m->kind = CAM_FACE; m->x = (int16_t)x; m->y = (int16_t)y; m->w = (int16_t)w;
      return true;
    }
    case 'H': {
      const char *s1 = (const char *)memchr(a, ' ', end - a);
      if (!s1 || !cam_field(a, s1, 1, sizeof m->version - 1, m->version)) return false;
      for (const char *p = m->version; *p; p++) {
        const char ch = *p;
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '.' || ch == '-' || ch == '_')) return false;
      }
      const size_t sn = (size_t)(end - s1 - 1);
      const char *st = s1 + 1;
      if (sn == 2 && memcmp(st, "on", 2) == 0) m->state = CAM_ST_ON;
      else if (sn == 3 && memcmp(st, "off", 3) == 0) m->state = CAM_ST_OFF;
      else if (sn == 3 && memcmp(st, "err", 3) == 0) m->state = CAM_ST_ERR;
      else return false;
      m->kind = CAM_HEART;
      return true;
    }
    case 'C':
      if (cn != 3 || (a[0] != '0' && a[0] != '1')) return false;
      m->kind = CAM_CTRL; m->on = a[0] == '1';
      return true;
    case 'U': {
      const size_t an = (size_t)(end - a);
      if (an == 2 && memcmp(a, "ok", 2) == 0) { m->kind = CAM_UPD_OK; return true; }
      if (an > 4 && memcmp(a, "err ", 4) == 0) {
        if (!cam_field(a + 4, end, 1, sizeof m->text - 1, m->text)) return false;
        m->kind = CAM_UPD_ERR;
        return true;
      }
      int pct;
      if (!cam_num(a, end, 100, &pct)) return false;
      m->kind = CAM_UPD_PCT; m->pct = (uint8_t)pct;
      return true;
    }
    default:
      return false;
  }
}

// Sammelt Bytes zu Zeilen. feed() == true: line()/len() enthalten eine vollständige Zeile (ohne '\n' und '\r'),
// gültig bis zum nächsten feed(). Zu lange Zeilen werden bis zum nächsten '\n' verworfen.
struct CamLineBuf {
  char buf[CAM_LINE_MAX + 1];
  size_t n = 0, done = 0;
  bool over = false;
  bool feed(char c) {
    if (c == '\r') return false;
    if (c == '\n') {
      const bool ok = !over && n > 0;
      buf[n] = '\0';
      done = n;
      n = 0;
      over = false;
      return ok;
    }
    if (over) return false;
    if (n >= CAM_LINE_MAX) { over = true; return false; }
    buf[n++] = c;
    return false;
  }
  const char *line() const { return buf; }
  size_t len() const { return done; }
};
```

Hinweis: In `feed` überschreibt das nächste Zeichen `buf[0]`. `line()` ist also nur bis zum nächsten `feed()` gültig. So steht es auch im Kommentar.

- [ ] **Step 4: Test grün sehen** (Befehl aus Step 2). Expected: `11 Tests 0 Failures`.

- [ ] **Step 5: Gegenprobe** – in `cam_parse` `if (cam_xor(line, cn) != …) return false;` vorübergehend auskommentieren. Expected: `test_checksum_rules` FAIL. Zurücksetzen, wieder grün.

- [ ] **Step 6: Spec-Zeile 24 korrigieren** (200 → 400, siehe Files) und Commit

```bash
git add src/logic/cam_proto.h test/test_cam_proto/test_main.cpp docs/superpowers/specs/2026-10-26-kamera-firmware-design.md docs/FORTSCHRITT.md
git commit -m "feat(cam): Zeilenprotokoll cam_proto.h mit nativen Tests"
```

---

### Task 3: `cam_xiao.h` (Box, Probelauf) und S3 in `ota_policy.h`

**Files:**
- Create/complete: `src/logic/cam_xiao.h` (der `CamFace`-Teil existiert seit Task 1)
- Modify: `src/logic/ota_policy.h:14` und `:25-28`
- Modify: `test/test_cam_proto/test_main.cpp` (Tests anhängen), `test/test_ota_policy/test_main.cpp` (ein Test)

**Interfaces:**
- Produces:

```cpp
struct CamFace { int16_t x, y, w; };
bool cam_box_to_face(int x1, int y1, int x2, int y2, int img_w, int img_h, CamFace *f);
enum CamHealth : uint8_t { CAM_HEALTH_WAIT = 0, CAM_HEALTH_VALID, CAM_HEALTH_ROLLBACK };
CamHealth cam_health(uint32_t up_ms, bool cam_ok, uint32_t beats);
constexpr uint16_t OTA_CHIP_ESP32S3 = 9;
OtaImg ota_image_check(const uint8_t *b, size_t n, const char *project, char *elf_hex, uint16_t chip = OTA_CHIP_ESP32);
```

- [ ] **Step 1: Tests anhängen** – in `test/test_cam_proto/test_main.cpp` oben `#include "logic/cam_xiao.h"` ergänzen, vor `main`:

```cpp
void test_box_center() {
  CamFace f;
  TEST_ASSERT_TRUE(cam_box_to_face(100, 80, 220, 200, 320, 240, &f));
  TEST_ASSERT_EQUAL(500, f.x);   // (100+220)/2 = 160 von 320
  TEST_ASSERT_EQUAL(583, f.y);   // 140 von 240
  TEST_ASSERT_EQUAL(375, f.w);   // 120 von 320
}
void test_box_clamped_and_degenerate() {
  CamFace f;
  TEST_ASSERT_TRUE(cam_box_to_face(-10, -10, 400, 300, 320, 240, &f));
  TEST_ASSERT_EQUAL(500, f.x); TEST_ASSERT_EQUAL(500, f.y); TEST_ASSERT_EQUAL(1000, f.w);
  TEST_ASSERT_FALSE(cam_box_to_face(50, 10, 50, 20, 320, 240, &f));   // Breite 0
  TEST_ASSERT_FALSE(cam_box_to_face(60, 10, 50, 20, 320, 240, &f));   // vertauscht
  TEST_ASSERT_FALSE(cam_box_to_face(0, 0, 10, 10, 0, 240, &f));       // Bildgröße 0
}
void test_health_decision() {
  TEST_ASSERT_EQUAL(CAM_HEALTH_WAIT, cam_health(19999, true, 10));
  TEST_ASSERT_EQUAL(CAM_HEALTH_VALID, cam_health(20000, true, 5));
  TEST_ASSERT_EQUAL(CAM_HEALTH_WAIT, cam_health(20000, true, 4));
  TEST_ASSERT_EQUAL(CAM_HEALTH_WAIT, cam_health(30000, false, 10));
  TEST_ASSERT_EQUAL(CAM_HEALTH_ROLLBACK, cam_health(60000, false, 10));
  TEST_ASSERT_EQUAL(CAM_HEALTH_ROLLBACK, cam_health(60000, true, 2));
  TEST_ASSERT_EQUAL(CAM_HEALTH_VALID, cam_health(60000, true, 5));   // gesund geht vor Zeitablauf
}
```

und in `main` `RUN_TEST(test_box_center); RUN_TEST(test_box_clamped_and_degenerate); RUN_TEST(test_health_decision);`.

In `test/test_ota_policy/test_main.cpp` vor `main`:

```cpp
void test_image_chip_s3() {
  char elf[65];
  make_img(9, "cam");
  TEST_ASSERT_EQUAL(OTA_IMG_CHIP, ota_image_check(img, sizeof img, "cam", elf));                    // Vorgabe bleibt ESP32
  TEST_ASSERT_EQUAL(OTA_IMG_OK, ota_image_check(img, sizeof img, "cam", elf, OTA_CHIP_ESP32S3));
  make_img(0, "cam");
  TEST_ASSERT_EQUAL(OTA_IMG_CHIP, ota_image_check(img, sizeof img, "cam", elf, OTA_CHIP_ESP32S3));
}
```

und `RUN_TEST(test_image_chip_s3);`.

- [ ] **Step 2: Rot sehen**

```powershell
$env:PATH = "C:\Users\madma\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin;" + $env:PATH
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" test -e native -f test_cam_proto -f test_ota_policy 2>&1 | Select-String 'error|FAIL|PASSED|ERRORED|Tests' | Select-Object -Last 15
```

Expected: ERRORED (`cam_health`, `OTA_CHIP_ESP32S3` fehlen).

- [ ] **Step 3: `src/logic/cam_xiao.h` vollständig**

```cpp
#pragma once
#include <stdint.h>

// Reine Logik der XIAO-Firmware (cam/): Gesichtsbox -> Protokollwerte, Entscheidung im Probelauf nach einem Update.
// Ohne Arduino-/IDF-Includes, Host-Test test/test_cam_proto.

struct CamFace { int16_t x, y, w; };   // Mitte x/y und Breite, je 0..1000 bezogen auf das Bild

static inline int cam_clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

// Box (x1,y1)-(x2,y2) in Bildpixeln. false bei leerer Box oder Bildgröße 0.
static inline bool cam_box_to_face(int x1, int y1, int x2, int y2, int img_w, int img_h, CamFace *f) {
  if (img_w <= 0 || img_h <= 0) return false;
  x1 = cam_clampi(x1, 0, img_w); x2 = cam_clampi(x2, 0, img_w);
  y1 = cam_clampi(y1, 0, img_h); y2 = cam_clampi(y2, 0, img_h);
  if (x2 <= x1 || y2 <= y1) return false;
  f->x = (int16_t)(((long)(x1 + x2) * 500) / img_w);
  f->y = (int16_t)(((long)(y1 + y2) * 500) / img_h);
  f->w = (int16_t)(((long)(x2 - x1) * 1000) / img_w);
  return true;
}

// Probelauf nach einem Update: gilt als gut nach 20 s ohne Kamerafehler mit mindestens 5 Lebenszeichen,
// sonst Rollback nach 60 s. cam_ok = Kamera seit dem Start einmal gelaufen und seitdem kein Fehler.
enum CamHealth : uint8_t { CAM_HEALTH_WAIT = 0, CAM_HEALTH_VALID, CAM_HEALTH_ROLLBACK };
constexpr uint32_t CAM_HEALTH_MIN_MS = 20000;
constexpr uint32_t CAM_HEALTH_MAX_MS = 60000;
constexpr uint32_t CAM_HEALTH_BEATS = 5;

static inline CamHealth cam_health(uint32_t up_ms, bool cam_ok, uint32_t beats) {
  if (up_ms >= CAM_HEALTH_MIN_MS && cam_ok && beats >= CAM_HEALTH_BEATS) return CAM_HEALTH_VALID;
  if (up_ms >= CAM_HEALTH_MAX_MS) return CAM_HEALTH_ROLLBACK;
  return CAM_HEALTH_WAIT;
}
```

- [ ] **Step 4: `src/logic/ota_policy.h` ändern**

Zeile 14 ersetzen durch:

```cpp
constexpr uint16_t OTA_CHIP_ESP32 = 0;          // ESP32 (CYD)
constexpr uint16_t OTA_CHIP_ESP32S3 = 9;        // ESP32-S3 (XIAO-Kamera, cam/)
```

Signatur und Chip-Vergleich (Zeilen 24–28):

```cpp
// Prüft die ersten Bytes eines Images. elf_hex (65 Byte) bekommt bei OTA_IMG_OK die ELF-SHA-256 des Images (64 Hexziffern).
// chip: erwartete Chip-ID aus dem Image-Kopf (CYD: ESP32, cam/: ESP32-S3).
static inline OtaImg ota_image_check(const uint8_t *b, size_t n, const char *project, char *elf_hex, uint16_t chip = OTA_CHIP_ESP32) {
  if (!b || n < OTA_IMG_CHECK_BYTES) return OTA_IMG_SHORT;
  if (b[0] != 0xE9) return OTA_IMG_MAGIC;
  if ((uint16_t)(b[12] | (b[13] << 8)) != chip) return OTA_IMG_CHIP;
```

- [ ] **Step 5: Grün sehen** (Befehl aus Step 2). Expected: beide Gruppen 0 Failures.

- [ ] **Step 6: Gegenprobe** – in `cam_health` die Bedingung `beats >= CAM_HEALTH_BEATS` entfernen. Expected: `test_health_decision` FAIL. Zurücksetzen.

- [ ] **Step 7: CYD baut weiter** (`ota_pull.cpp` ruft ohne Chip auf, Vorgabe ESP32):

```powershell
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" run -e cyd2usb 2>&1 | Select-String 'error|SUCCESS|FAILED' | Select-Object -Last 5
```

- [ ] **Step 8: Commit**

```bash
git add src/logic/cam_xiao.h src/logic/ota_policy.h test/test_cam_proto/test_main.cpp test/test_ota_policy/test_main.cpp docs/FORTSCHRITT.md
git commit -m "feat(cam): Box-Umrechnung, Probelauf-Entscheidung, S3-Chip in der Image-Pruefung"
```

---

### Task 4: XIAO-Firmware – Link, Updater, Ablauf

**Files:**
- Create: `cam/src/link.h`, `cam/src/link.cpp`, `cam/src/updater.h`, `cam/src/updater.cpp`
- Replace: `cam/src/main.cpp` (Spike → endgültig)
- Modify: `cam/src/detect.h`/`detect.cpp`: `CamFace` nur noch aus `logic/cam_xiao.h`, falls im Spike doppelt

**Interfaces:**
- Consumes: alles aus `cam_proto.h`, `cam_xiao.h`, `ota_policy.h` (`ota_redirect_ok`, `ota_image_check`, `OTA_IMG_CHECK_BYTES`, `ota_image_error`, `ota_progress_pct`, `OTA_CHIP_ESP32S3`), `camera_*`, `detect_*`.
- Produces: `void link_begin(); void link_send(const char *s, size_t n, bool mirror = true); bool link_poll(CamMsg *m);` · `void updater_boot(); void updater_run(const CamMsg &cmd, void (*beat)()); void updater_health_tick(uint32_t now, bool cam_ok, uint32_t beats);`

- [ ] **Step 1: `cam/src/link.h` / `link.cpp`**

```cpp
// link.h
#pragma once
#include <stddef.h>
#include "logic/cam_proto.h"
void link_begin();
void link_send(const char *s, size_t n, bool mirror = true);   // UART zum CYD, mirror = zusätzlich auf USB (Messen)
bool link_poll(CamMsg *m);   // true = Befehl (C oder U-Auftrag) vom CYD oder vom USB-Seriell
```

```cpp
// link.cpp
#include <Arduino.h>
#include "link.h"
#include "cam_config.h"

static HardwareSerial g_uart(1);
static CamLineBuf g_ub, g_sb;

void link_begin() {
  g_uart.setRxBufferSize(1024);   // eine U-Zeile (bis 400 Zeichen) muss eine Erkennung (bis ~250 ms) überstehen
  g_uart.begin(CAM_UART_BAUD, SERIAL_8N1, CAM_UART_RX, CAM_UART_TX);
}

void link_send(const char *s, size_t n, bool mirror) {
  g_uart.write((const uint8_t *)s, n);
  if (mirror) Serial.write((const uint8_t *)s, n);
}

// Befehle kommen vom CYD (UART) oder zum Testen vom PC (USB, tools/cam-serial.py).
static bool poll_one(Stream &st, CamLineBuf &lb, CamMsg *m) {
  while (st.available()) {
    if (!lb.feed((char)st.read())) continue;
    if (cam_parse(lb.line(), lb.len(), m) && (m->kind == CAM_CTRL || m->kind == CAM_UPD_CMD)) return true;
  }
  return false;
}

bool link_poll(CamMsg *m) { return poll_one(g_uart, g_ub, m) || poll_one(Serial, g_sb, m); }
```

- [ ] **Step 2: `cam/src/updater.h` / `updater.cpp`**

```cpp
// updater.h
#pragma once
#include <stdint.h>
#include "logic/cam_proto.h"
void updater_boot();                                               // Zustand der laufenden Partition lesen
void updater_run(const CamMsg &cmd, void (*beat)());              // blockiert; bei Erfolg Neustart, sonst "U err" und zurück
void updater_health_tick(uint32_t now, bool cam_ok, uint32_t beats);   // Probelauf: bestätigen oder zurückrollen
```

```cpp
// updater.cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include "updater.h"
#include "link.h"
#include "cam_config.h"
#include "logic/ota_policy.h"
#include "logic/cam_xiao.h"

// WLAN-Update des XIAO (Spec: Updates). Ablauf wie src/ota/ota_pull.cpp am CYD, aber Auftrag (URL, SHA-256, WLAN) vom CYD
// über die U-Zeile. Die SHA-256 kommt aus dem Manifest, das der CYD per geprüftem TLS geladen hat. Sie legt den Inhalt fest.

namespace {
constexpr size_t kChunk = 4096;
constexpr uint32_t kStallMs = 15000;
constexpr uint32_t kWifiMs = 20000;
constexpr int kMaxRedirects = 3;
bool g_pending = false;   // läuft auf Probe (PENDING_VERIFY)

#if ESP_ARDUINO_VERSION_MAJOR >= 3
const char *project_name() { return esp_app_get_description()->project_name; }
#define SHA_START(c) mbedtls_sha256_starts(c, 0)
#define SHA_UPDATE(c, b, n) mbedtls_sha256_update(c, b, n)
#define SHA_FINISH(c, d) mbedtls_sha256_finish(c, d)
#else
const char *project_name() { return esp_ota_get_app_description()->project_name; }
#define SHA_START(c) mbedtls_sha256_starts_ret(c, 0)
#define SHA_UPDATE(c, b, n) mbedtls_sha256_update_ret(c, b, n)
#define SHA_FINISH(c, d) mbedtls_sha256_finish_ret(c, d)
#endif

void send_err(const char *why) {
  char b[64];
  const size_t n = cam_build_upd_err(why, b, sizeof b);
  link_send(b, n);
}
void send_pct(int pct) {
  char b[16];
  const size_t n = cam_build_upd_pct(pct, b, sizeof b);
  link_send(b, n);
}

void setup_tls(WiFiClientSecure &tls) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  tls.useBuiltinCACertBundle();
#else
  tls.setInsecure();   // Kern 2.x ohne eingebautes Zertifikatsbündel: Der Inhalt ist über die SHA-256 festgelegt
#endif
}

// GET mit selbst verfolgten Umleitungen; jede Adresse muss ota_redirect_ok() bestehen.
bool open_follow(HTTPClient &http, WiFiClientSecure &tls, const char *url, int *code, char *why, size_t wn) {
  String cur = url;
  static const char *keys[] = {"Location"};
  for (int hop = 0; hop <= kMaxRedirects; hop++) {
    if (!ota_redirect_ok(cur.c_str())) { strlcpy(why, "Umleitung abgelehnt", wn); return false; }
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (!http.begin(tls, cur)) { strlcpy(why, "URL", wn); return false; }
    http.collectHeaders(keys, 1);
    *code = http.GET();
    if (*code == 301 || *code == 302 || *code == 303 || *code == 307 || *code == 308) {
      cur = http.header("Location");
      http.end();
      continue;
    }
    if (*code <= 0) { strlcpy(why, "Verbindung", wn); http.end(); return false; }
    return true;
  }
  strlcpy(why, "zu viele Umleitungen", wn);
  return false;
}

void hex_of(const uint8_t *b, size_t n, char *out) {
  static const char k[] = "0123456789abcdef";
  for (size_t i = 0; i < n; i++) { out[2 * i] = k[b[i] >> 4]; out[2 * i + 1] = k[b[i] & 15]; }
  out[2 * n] = '\0';
}

void wifi_off() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// true = Image geschrieben und gesetzt; sonst why gefüllt
bool download(const CamMsg &cmd, void (*beat)(), char *why, size_t wn) {
  WiFiClientSecure tls;
  setup_tls(tls);
  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(15000);
  http.setConnectTimeout(6000);
  int code = 0;
  if (!open_follow(http, tls, cmd.url, &code, why, wn)) return false;
  if (code != 200) { snprintf(why, wn, "HTTP %d", code); http.end(); return false; }
  const int total = http.getSize();
  if (total < (int)OTA_IMG_CHECK_BYTES) { strlcpy(why, "Groesse unbekannt", wn); http.end(); return false; }

  uint8_t *buf = (uint8_t *)malloc(kChunk);
  if (!buf) { strlcpy(why, "kein Speicher", wn); http.end(); return false; }
  WiFiClient *s = http.getStreamPtr();
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  SHA_START(&sha);

  uint32_t done = 0, last_data = millis(), last_beat = millis();
  int last_pct = -1;
  bool started = false, fail = false;
  size_t have = 0;
  while (done < (uint32_t)total && !fail) {
    if (millis() - last_beat >= CAM_HEART_MS) { last_beat = millis(); beat(); }
    const int avail = s->available();
    if (avail <= 0) {
      if (!http.connected() && !s->available()) { strlcpy(why, "Verbindung weg", wn); fail = true; }
      else if (millis() - last_data > kStallMs) { strlcpy(why, "Zeitueberschreitung", wn); fail = true; }
      else delay(5);
      continue;
    }
    size_t want = kChunk - have;
    if (want > (size_t)avail) want = avail;
    if (want > (uint32_t)total - done - have) want = (uint32_t)total - done - have;
    const int r = s->read(buf + have, want);
    if (r <= 0) { delay(2); continue; }
    last_data = millis();
    have += r;
    if (!started) {
      if (have < OTA_IMG_CHECK_BYTES) continue;
      char elf[65];
      const OtaImg chk = ota_image_check(buf, have, project_name(), elf, OTA_CHIP_ESP32S3);
      if (chk != OTA_IMG_OK) { strlcpy(why, ota_image_error(chk), wn); fail = true; break; }
      if (!Update.begin(total, U_FLASH)) { strlcpy(why, "Update.begin", wn); fail = true; break; }
      started = true;
    }
    SHA_UPDATE(&sha, buf, have);
    if (Update.write(buf, have) != have) { strlcpy(why, "Schreiben", wn); fail = true; break; }
    done += have;
    have = 0;
    const int pct = ota_progress_pct(done, total);
    if (pct / 5 != last_pct / 5) { last_pct = pct; send_pct(pct); }
    delay(1);
  }
  http.end();
  free(buf);
  uint8_t digest[32];
  SHA_FINISH(&sha, digest);
  mbedtls_sha256_free(&sha);
  if (!fail && done != (uint32_t)total) { strlcpy(why, "unvollstaendig", wn); fail = true; }
  if (!fail) {
    char got[65];
    hex_of(digest, 32, got);
    if (strcmp(got, cmd.sha) != 0) { strlcpy(why, "SHA-256 falsch", wn); fail = true; }
  }
  if (fail) { if (started) Update.abort(); return false; }
  if (!Update.end(true)) { strlcpy(why, "Update.end", wn); return false; }
  return true;
}
}  // namespace

void updater_boot() {
  const esp_partition_t *run = esp_ota_get_running_partition();
  esp_ota_img_states_t st = ESP_OTA_IMG_UNDEFINED;
  if (run) esp_ota_get_state_partition(run, &st);
  g_pending = (st == ESP_OTA_IMG_PENDING_VERIFY);
  Serial.printf("# ota: Partition %s, Zustand %d\n", run ? run->label : "?", (int)st);
}

void updater_run(const CamMsg &cmd, void (*beat)()) {
  if (!ota_redirect_ok(cmd.url)) { send_err("URL abgelehnt"); return; }
  send_pct(0);
  WiFi.mode(WIFI_STA);
  WiFi.begin(cmd.ssid, cmd.pass);
  const uint32_t t0 = millis();
  uint32_t last_beat = t0;
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < kWifiMs) {
    if (millis() - last_beat >= CAM_HEART_MS) { last_beat = millis(); beat(); }
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) { wifi_off(); send_err("kein WLAN"); return; }
  char why[40] = "";
  const bool ok = download(cmd, beat, why, sizeof why);
  wifi_off();
  if (!ok) { send_err(why[0] ? why : "Fehler"); return; }
  send_pct(100);
  Serial.println("# ota: ok, Neustart");
  delay(500);
  ESP.restart();
}

void updater_health_tick(uint32_t now, bool cam_ok, uint32_t beats) {
  if (!g_pending) return;
  switch (cam_health(now, cam_ok, beats)) {
    case CAM_HEALTH_VALID: {
      g_pending = false;
      esp_ota_mark_app_valid_cancel_rollback();
      char b[16];
      const size_t n = cam_build_upd_ok(b, sizeof b);
      link_send(b, n);
      Serial.println("# ota: neue Firmware bestaetigt");
      break;
    }
    case CAM_HEALTH_ROLLBACK:
      Serial.println("# ota: Probelauf gescheitert, Rollback");
      delay(200);
      esp_ota_mark_app_invalid_rollback_and_reboot();
      break;
    default:
      break;
  }
}
```

Hinweis für den Umsetzer: Meldet der Kern 3 `WiFiClient` als nicht passend zu `getStreamPtr()` (dort `NetworkClient *`), nimm `auto *s = http.getStreamPtr();`.

- [ ] **Step 3: `cam/src/main.cpp` (endgültig)**

```cpp
#include <Arduino.h>
#include <esp_camera.h>
#include "cam_config.h"
#include "camera.h"
#include "detect.h"
#include "link.h"
#include "updater.h"
#include "logic/cam_proto.h"
#include "logic/cam_xiao.h"

// XIAO ESP32S3 Sense: Gesichter erkennen und dem CYD melden (Spec docs/superpowers/specs/2026-10-26-kamera-firmware-design.md).
// Bilder verlassen den XIAO nie; WLAN nur während eines Updates.

// Nach einem Update bestätigt erst der Probelauf das Image (updater_health_tick), nicht der Core beim Start.
extern "C" bool verifyRollbackLater() { return true; }

static bool g_want_on = true;     // Vorgabe an; der CYD schickt nach jedem Neustart C 1/C 0
static CamState g_state = CAM_ST_OFF;
static bool g_ever_ok = false, g_had_err = false;
static uint32_t g_last_heart = 0, g_last_retry = 0, g_beats = 0, g_fb_fail = 0;
static uint32_t g_stat_t0 = 0, g_frames = 0, g_ms_sum = 0;
static CamFace g_last = {0, 0, 0};
static bool g_has = false;

static void send_heart() {
  char b[48];
  const size_t n = cam_build_heart(CAM_FW_VERSION, g_state, b, sizeof b);
  link_send(b, n);
  g_beats++;
}

static void set_err() {
  camera_stop();
  g_state = CAM_ST_ERR;
  g_had_err = true;
  g_last_retry = millis();
}

static void apply_camera() {
  if (!g_want_on) {
    camera_stop();
    g_state = CAM_ST_OFF;
    return;
  }
  if (camera_running()) { g_state = CAM_ST_ON; return; }
  if (camera_start()) { g_state = CAM_ST_ON; g_ever_ok = true; g_fb_fail = 0; }
  else set_err();
}

void setup() {
  Serial.begin(115200);
  link_begin();
  updater_boot();
  Serial.printf("# cam %s psram=%u detect=%d\n", CAM_FW_VERSION, (unsigned)ESP.getPsramSize(), detect_init());
  apply_camera();
  send_heart();
  g_last_heart = g_stat_t0 = millis();
}

void loop() {
  const uint32_t now = millis();
  CamMsg m;
  while (link_poll(&m)) {
    if (m.kind == CAM_CTRL) {
      g_want_on = m.on;
      apply_camera();
      send_heart();
    } else if (m.kind == CAM_UPD_CMD) {
      camera_stop();               // Speicher und Rechenzeit fürs Update
      g_state = CAM_ST_OFF;
      updater_run(m, send_heart);  // kehrt nur bei Fehler zurück
      apply_camera();
    }
  }
  if (g_want_on && g_state == CAM_ST_ERR && now - g_last_retry >= CAM_RETRY_MS) apply_camera();
  if (now - g_last_heart >= CAM_HEART_MS) { g_last_heart = now; send_heart(); }
  updater_health_tick(now, g_ever_ok && !g_had_err, g_beats);

  if (camera_running()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      if (++g_fb_fail >= 10) set_err();   // Sensor liefert nicht mehr
      delay(10);
      return;
    }
    g_fb_fail = 0;
    CamFace f;
    uint32_t ms = 0;
    g_has = detect_face(fb, &f, &ms);
    esp_camera_fb_return(fb);
    if (g_has) g_last = f;
    char b[32];
    const size_t n = g_has ? cam_build_face(f.x, f.y, f.w, b, sizeof b) : cam_build_noface(b, sizeof b);
    link_send(b, n, false);   // F/N nicht auf USB, dafür die Messzeile unten
    g_frames++;
    g_ms_sum += ms;
  } else {
    delay(20);
  }
  if (now - g_stat_t0 >= 1000) {
    Serial.printf("# fps=%u ms=%u face=%d %d,%d,%d\n", (unsigned)g_frames, (unsigned)(g_frames ? g_ms_sum / g_frames : 0),
                  g_has, g_last.x, g_last.y, g_last.w);
    g_stat_t0 = now;
    g_frames = 0;
    g_ms_sum = 0;
  }
}
```

- [ ] **Step 4: Bauen** (Befehl aus Task 1 Step 6). Expected: `SUCCESS`. Zusätzlich das Rollback-Testbild bauen, das in Task 5 gebraucht wird, und danach wieder normal:

```powershell
$env:PLATFORMIO_BUILD_FLAGS = "-DCAM_TEST_BROKEN -DCAM_FW_VERSION=\`"0.1.0-broken\`""
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" run -d cam 2>&1 | Select-String 'error|SUCCESS|FAILED' | Select-Object -Last 3
Copy-Item cam\.pio\build\cam\firmware.bin $env:TEMP\cam-broken.bin
Remove-Item Env:PLATFORMIO_BUILD_FLAGS
& "C:\Users\madma\AppData\Roaming\Python\Python314\Scripts\pio.exe" run -d cam 2>&1 | Select-String 'error|SUCCESS|FAILED' | Select-Object -Last 3
```

- [ ] **Step 5: Commit**

```bash
git add cam/src docs/FORTSCHRITT.md
git commit -m "feat(cam): XIAO-Firmware mit Protokoll, Kamera an/aus, WLAN-Update und Rollback"
```

---

### Task 5: Flashen und am Gerät abnehmen (vor dem Löten)

Diesen Task führt **Opus selbst** mit dem Nutzer durch, nicht per Subagent. Jede Nutzeraktion wird vorher angesagt.

- [ ] **Step 1: Volle Prüfung** (einmal): `pio test -e native` (Summe), `node --test tools/*.test.mjs`, `pio run -e cyd2usb`, `pio run -d cam`. Ausgaben gefiltert.

- [ ] **Step 2: Flash per USB-C** (Ansage und Befehl wie Task 1 Step 8).

- [ ] **Step 3: Protokoll und Lebenszeichen**

```powershell
& "C:\Users\madma\AppData\Local\Python\pythoncore-3.14-64\python.exe" tools/cam-serial.py COMx --seconds 20 --send "C 0" --send "C 1" --send-at 6
```

Expected: `# ota: Partition app0, Zustand -1` oder `2`, `H 0.1.0 on*..` alle 2 s, nach `C 0` `H 0.1.0 off`, `fps=0`, nach `C 1` wieder `on` und fps > 0.

- [ ] **Step 4: Messwerte – NUTZER-SCHRITT.** Ansage: „1) Setz dich in normaler Entfernung davor. 2) Komm langsam ganz nah an die Linse. 3) Geh nach links aus dem Bild, dann nach rechts. Ich sage dir jeweils, wann.“ Mitschnitt 60 s. Notieren: fps, ms/Bild, größte Entfernung mit sicherer Erkennung, `w` in normaler Entfernung und ganz nah (→ `CAM_NEAR_W` für Phase B), Richtung von `x` beim Gang nach links (→ `CAM_MIRROR_X` für Phase B). Werte in `FORTSCHRITT.md` und in den Abschnitt „Hängt an der Hardware“ der Spec.

- [ ] **Step 5: WLAN-Update und Rollback – braucht Freigabe des Nutzers.** Für die Datei auf GitHub wird ein **Pre-Release** `cam-test-<datum>` in `M4dm0nky/cyd-buddy` angelegt (nicht `--latest`, der CYD sieht es also nicht). Das wirkt nach außen, deshalb vorher fragen. Das WLAN-Passwort gibt der Nutzer an, oder der Schritt wird mit seiner Zustimmung ausgelassen. Ablauf:
  1. `cam\.pio\build\cam\firmware.bin` als `cam.bin` und `cam-broken.bin` hochladen: `gh release create cam-test-20261029 cam.bin cam-broken.bin --prerelease --repo M4dm0nky/cyd-buddy --title "cam test"`. SHA-256 mit `Get-FileHash -Algorithm SHA256` (kleingeschrieben).
  2. Gutes Update: `tools/cam-serial.py COMx --seconds 120 --send "U\t<url cam.bin>\t<sha>\t<ssid>\t<pw>"`. Expected: `U 0` … `U 100`, Neustart, `Zustand 1`, nach ≥ 20 s `U ok*..` und `neue Firmware bestaetigt`. Nach erneutem Reset (R) `Zustand 2` auf der anderen Partition.
  3. Falsche SHA (ein Zeichen geändert): Expected: `U err SHA-256 falsch`, kein Neustart, Kamera wieder `on`.
  4. Rollback: `cam-broken.bin` mit richtiger SHA. Expected: Neustart, `H 0.1.0-broken err`, nach 60 s `Rollback`, Neustart auf der vorigen Partition mit `H 0.1.0 on`.
  5. Pre-Release danach löschen: `gh release delete cam-test-20261029 --yes --cleanup-tag --repo M4dm0nky/cyd-buddy`.

- [ ] **Step 6: Endstand flashen.** Läuft der XIAO nach dem Test auf `app1`, ist das in Ordnung. Wichtig ist nur, dass die gute Firmware läuft (`H 0.1.0 on`).

- [ ] **Step 7: Kassensturz** – kein `python`, `pio` oder `esptool` mehr aktiv (`Get-CimInstance Win32_Process | Where-Object { $_.Name -match 'python|pio|esptool' }`).

- [ ] **Step 8: Abschluss** – FORTSCHRITT-Eintrag mit allen Messwerten und Ergebnissen. Die Spec bekommt die Messwerte. Commit `docs(cam): Phase A am Geraet abgenommen`. Push des Branches nach Rückfrage. Nächster Schritt für den Nutzer: verlöten nach `docs/verkabelung.md`, danach Phase B.
