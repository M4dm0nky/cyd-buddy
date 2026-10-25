#include <unity.h>
#include "logic/pomodoro.h"
void setUp() {}
void tearDown() {}

static constexpr uint32_t MIN = 60000UL;

void test_start_only_15_25_45() {
  Pomo p{};
  TEST_ASSERT_EQUAL((int)PomoState::IDLE, (int)p.state);
  TEST_ASSERT_FALSE(pomo_start(p, 20, 0));
  TEST_ASSERT_EQUAL((int)PomoState::IDLE, (int)p.state);
  TEST_ASSERT_TRUE(pomo_start(p, 15, 0));
  TEST_ASSERT_EQUAL((int)PomoState::WORK, (int)p.state);
  TEST_ASSERT_TRUE(pomo_start(p, 25, 0));
  TEST_ASSERT_TRUE(pomo_start(p, 45, 0));
}
void test_work_break_idle() {
  Pomo p{};
  pomo_start(p, 25, 1000);
  TEST_ASSERT_EQUAL((int)PomoEvent::NONE, (int)pomo_tick(p, 1000 + 25 * MIN - 1));
  TEST_ASSERT_EQUAL((int)PomoState::WORK, (int)p.state);
  TEST_ASSERT_EQUAL((int)PomoEvent::BREAK_START, (int)pomo_tick(p, 1000 + 25 * MIN));
  TEST_ASSERT_EQUAL((int)PomoState::BREAK, (int)p.state);
  TEST_ASSERT_EQUAL((int)PomoEvent::NONE, (int)pomo_tick(p, 1000 + 25 * MIN + 1));
  TEST_ASSERT_EQUAL((int)PomoEvent::NONE, (int)pomo_tick(p, 1000 + 30 * MIN - 1));
  TEST_ASSERT_EQUAL((int)PomoEvent::BREAK_END, (int)pomo_tick(p, 1000 + 30 * MIN));
  TEST_ASSERT_EQUAL((int)PomoState::IDLE, (int)p.state);
  TEST_ASSERT_EQUAL((int)PomoEvent::NONE, (int)pomo_tick(p, 1000 + 60 * MIN));   // nicht automatisch neu
  TEST_ASSERT_EQUAL((int)PomoState::IDLE, (int)p.state);
}
void test_late_tick_counts_from_phase_end() {
  Pomo p{};
  pomo_start(p, 15, 0);
  TEST_ASSERT_EQUAL((int)PomoEvent::BREAK_START, (int)pomo_tick(p, 16 * MIN));   // 1 min zu spät abgefragt
  TEST_ASSERT_EQUAL_UINT32(4 * 60, pomo_remaining_s(p, 16 * MIN));             // Pause läuft seit 15:00
}
void test_stop() {
  Pomo p{};
  pomo_start(p, 45, 0);
  pomo_stop(p);
  TEST_ASSERT_EQUAL((int)PomoState::IDLE, (int)p.state);
  TEST_ASSERT_EQUAL((int)PomoEvent::NONE, (int)pomo_tick(p, 50 * MIN));
}
void test_progress_and_remaining() {
  Pomo p{};
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pomo_progress(p, 5));
  TEST_ASSERT_EQUAL_UINT32(0, pomo_remaining_s(p, 5));
  pomo_start(p, 25, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pomo_progress(p, 0));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, pomo_progress(p, 12 * MIN + 30000));
  TEST_ASSERT_EQUAL_UINT32(25 * 60, pomo_remaining_s(p, 0));
  TEST_ASSERT_EQUAL_UINT32(25 * 60, pomo_remaining_s(p, 1));      // aufgerundet
  TEST_ASSERT_EQUAL_UINT32(1, pomo_remaining_s(p, 25 * MIN - 1));
  pomo_tick(p, 25 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, pomo_progress(p, 25 * MIN));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.2f, pomo_progress(p, 26 * MIN));
  TEST_ASSERT_EQUAL_UINT32(5 * 60, pomo_remaining_s(p, 25 * MIN));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, pomo_progress(p, 40 * MIN));   // geklemmt (vor dem nächsten tick)
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_start_only_15_25_45);
  RUN_TEST(test_work_break_idle);
  RUN_TEST(test_late_tick_counts_from_phase_end);
  RUN_TEST(test_stop);
  RUN_TEST(test_progress_and_remaining);
  return UNITY_END();
}
