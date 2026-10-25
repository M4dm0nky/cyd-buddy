#pragma once
#include <stddef.h>
#include <stdint.h>

// UTF-8 -> ASCII für die eingebauten Schriften (nur 0x20..0x7E): ä→ae, ß→ss, „–“→"-", „…“→"...", sonst "?".
inline void ascii_de(const char *in, char *out, size_t n) {
  if (!out || n == 0) return;
  size_t o = 0;
  auto put = [&](const char *s) {
    while (*s && o + 1 < n) out[o++] = *s++;
  };
  const unsigned char *p = (const unsigned char *)(in ? in : "");
  while (*p && o + 1 < n) {
    const unsigned char c = *p;
    if (c < 0x80) { out[o++] = (char)c; p++; continue; }
    uint32_t cp = 0;
    int len = 0;
    if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; len = 3; }
    p++;
    for (int i = 0; i < len && (*p & 0xC0) == 0x80; i++, p++) cp = (cp << 6) | (*p & 0x3F);
    switch (cp) {
      case 0xE4: put("ae"); break;
      case 0xF6: put("oe"); break;
      case 0xFC: put("ue"); break;
      case 0xC4: put("Ae"); break;
      case 0xD6: put("Oe"); break;
      case 0xDC: put("Ue"); break;
      case 0xDF: put("ss"); break;
      case 0xE9: case 0xE8: put("e"); break;
      case 0xB0: put(" Grad"); break;
      case 0x2013: case 0x2014: put("-"); break;
      case 0x2026: put("..."); break;
      case 0x201E: case 0x201C: case 0x201D: put("\""); break;
      default: put("?"); break;
    }
  }
  out[o] = '\0';
}
