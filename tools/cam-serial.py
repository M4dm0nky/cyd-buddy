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
