#include <unity.h>
#include "logic/simon.h"
#include "logic/catch_game.h"
void setUp() {}
void tearDown() {}

static void play_round_correct(Simon &s) {
  simon_shown(s);
  for (uint8_t i = 0; i < s.len; i++) simon_press(s, s.seq[i]);
}
void test_simon_win() {
  Simon s; simon_start(s, 77);
  for (int i = 0; i < SIMON_WIN_LEN; i++) { TEST_ASSERT_EQUAL((int)SimonState::SHOW, (int)s.state); play_round_correct(s); }
  TEST_ASSERT_EQUAL((int)SimonState::WON, (int)s.state);
}
void test_simon_wrong_loses() {
  Simon s; simon_start(s, 77);
  simon_shown(s);
  simon_press(s, (uint8_t)((s.seq[0] + 1) % 4));
  TEST_ASSERT_EQUAL((int)SimonState::LOST, (int)s.state);
}
void test_simon_seq_range_and_seed() {
  Simon a, b; simon_start(a, 5); simon_start(b, 5);
  for (int i = 0; i < SIMON_WIN_LEN; i++) { TEST_ASSERT_TRUE(a.seq[i] < 4); TEST_ASSERT_EQUAL_UINT8(a.seq[i], b.seq[i]); }
}
void test_simon_press_ignored_in_show() {
  Simon s; simon_start(s, 1);
  simon_press(s, s.seq[0]);
  TEST_ASSERT_EQUAL((int)SimonState::SHOW, (int)s.state);
}
void test_catch_interval() {
  TEST_ASSERT_EQUAL_UINT32(1200, catch_jump_interval(0));
  TEST_ASSERT_EQUAL_UINT32(800, catch_jump_interval(5));
  TEST_ASSERT_EQUAL_UINT32(450, catch_jump_interval(20));
}
void test_catch_hit_miss_and_bounds() {
  CatchGame g; catch_start(g, 3, 0);
  for (uint32_t t = 0; t < 19000; t += 50) {
    catch_tick(g, t);
    TEST_ASSERT_TRUE(g.x >= CATCH_R && g.x <= CATCH_W - CATCH_R);
    TEST_ASSERT_TRUE(g.y >= CATCH_R && g.y <= CATCH_H - CATCH_R);
  }
  int16_t x = g.x, y = g.y;
  TEST_ASSERT_TRUE(catch_tap(g, x + 5, y, 19000));
  TEST_ASSERT_EQUAL_UINT16(1, g.hits);
  TEST_ASSERT_FALSE(catch_tap(g, (int16_t)(g.x + 100 > CATCH_W ? g.x - 100 : g.x + 100), g.y, 19010));
}
void test_catch_round_end_and_win() {
  CatchGame g; catch_start(g, 3, 0);
  for (int i = 0; i < 8; i++) catch_tap(g, g.x, g.y, 1000 + i);
  catch_tick(g, 19999);
  TEST_ASSERT_FALSE(g.over);
  catch_tick(g, 20000);
  TEST_ASSERT_TRUE(g.over);
  TEST_ASSERT_TRUE(catch_won(g));
  TEST_ASSERT_FALSE(catch_tap(g, g.x, g.y, 20001));   // nach Ende zählt nichts
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_simon_win);
  RUN_TEST(test_simon_wrong_loses);
  RUN_TEST(test_simon_seq_range_and_seed);
  RUN_TEST(test_simon_press_ignored_in_show);
  RUN_TEST(test_catch_interval);
  RUN_TEST(test_catch_hit_miss_and_bounds);
  RUN_TEST(test_catch_round_end_and_win);
  return UNITY_END();
}
