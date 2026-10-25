#include <unity.h>
#include <initializer_list>
#include "logic/claude_face.h"
void setUp() {}
void tearDown() {}

static void expect(ClaudeState cs, Emotion e, uint8_t fx) {
  const ClaudeFace f = claude_face(cs, false, false, Emotion::BORED);
  TEST_ASSERT_EQUAL((int)e, (int)f.emo);
  TEST_ASSERT_EQUAL_UINT8(fx, f.fx);
}

void test_table() {
  expect(ClaudeState::LISTENING, Emotion::SURPRISED, CFX_NONE);
  expect(ClaudeState::THINKING, Emotion::THINKING, CFX_NONE);
  expect(ClaudeState::READING, Emotion::THINKING, CFX_JUMP);
  expect(ClaudeState::CODING, Emotion::FOCUSED, CFX_TYPING);
  expect(ClaudeState::RUNNING, Emotion::FOCUSED, CFX_RING);
  expect(ClaudeState::FAILED, Emotion::SCARED, CFX_NONE);
  expect(ClaudeState::NEEDS_YOU, Emotion::CONTENT, CFX_STRAIGHT);
  expect(ClaudeState::DONE, Emotion::EXCITED, CFX_HAND);
}

void test_off_and_idle_keep_pet_face() {
  for (ClaudeState cs : {ClaudeState::OFF, ClaudeState::IDLE}) {
    TEST_ASSERT_FALSE(claude_busy(cs));
    const ClaudeFace f = claude_face(cs, false, false, Emotion::SLEEPING);
    TEST_ASSERT_EQUAL((int)Emotion::SLEEPING, (int)f.emo);
    TEST_ASSERT_EQUAL_UINT8(CFX_NONE, f.fx);
  }
}

void test_touch_feeling_wins() {
  for (int i = (int)ClaudeState::LISTENING; i <= (int)ClaudeState::DONE; i++) {
    const ClaudeFace f = claude_face((ClaudeState)i, false, true, Emotion::LOVE);
    TEST_ASSERT_EQUAL((int)Emotion::LOVE, (int)f.emo);
    TEST_ASSERT_EQUAL_UINT8(CFX_NONE, f.fx);   // auch keine Zusatzeffekte
  }
}

void test_pomodoro_wins() {
  for (int i = (int)ClaudeState::LISTENING; i <= (int)ClaudeState::DONE; i++) {
    const ClaudeFace f = claude_face((ClaudeState)i, true, false, Emotion::FOCUSED);
    TEST_ASSERT_EQUAL((int)Emotion::FOCUSED, (int)f.emo);
    TEST_ASSERT_EQUAL_UINT8(CFX_NONE, f.fx);
  }
}

// Gegenprobe: ohne Vorrang zeigt jeder arbeitende Zustand ein Claude-Gesicht, nie das Haustier-Gefuehl.
void test_busy_overrides_pet_and_sleep() {
  for (int i = (int)ClaudeState::LISTENING; i <= (int)ClaudeState::DONE; i++) {
    TEST_ASSERT_TRUE(claude_busy((ClaudeState)i));
    const ClaudeFace f = claude_face((ClaudeState)i, false, false, Emotion::SLEEPING);
    TEST_ASSERT_NOT_EQUAL((int)Emotion::SLEEPING, (int)f.emo);
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_table);
  RUN_TEST(test_off_and_idle_keep_pet_face);
  RUN_TEST(test_touch_feeling_wins);
  RUN_TEST(test_pomodoro_wins);
  RUN_TEST(test_busy_overrides_pet_and_sleep);
  return UNITY_END();
}
