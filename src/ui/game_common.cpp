#include "game_common.h"
#include "eyes.h"

void game_show_result(LGFX &lcd, bool won) {
  const Expression x = eye_expression(won ? Emotion::EXCITED : Emotion::BORED);
  lcd.startWrite();
  lcd.fillScreen(TFT_BLACK);
  eyes_draw_pair(lcd, 160, 120, x.shape, x.color, 1.2f);
  lcd.endWrite();
}
