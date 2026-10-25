#include "fonts.h"
#include <stdint.h>

// Schrift-Arrays liegen in der Bibliothek olikraus/U8g2 (u8g2_fonts.c); nur die benutzten werden gelinkt.
extern "C" {
extern const uint8_t u8g2_font_helvB10_tf[];
extern const uint8_t u8g2_font_helvB14_tf[];
extern const uint8_t u8g2_font_helvB24_tf[];
}

const lgfx::U8g2font FONT_S(u8g2_font_helvB10_tf);
const lgfx::U8g2font FONT_M(u8g2_font_helvB14_tf);
const lgfx::U8g2font FONT_L(u8g2_font_helvB24_tf);
