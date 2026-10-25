#include <unity.h>
#include <string.h>
#include "logic/claude_state.h"
void setUp() {}
void tearDown() {}

static constexpr uint32_t MIN = 60000;

static ClaudeState after(const char *e, const char *tool = "", const char *n = "", const char *sid = "s1", bool i = false,
                         uint32_t t = 1000) {
  ClaudeLogic L;
  claude_event(L, e, tool, n, sid, i, t);
  return claude_state(L, t);
}

void test_initial_is_off() {
  ClaudeLogic L;
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)claude_state(L, 0));
  TEST_ASSERT_FALSE(claude_active(L, 0));
  TEST_ASSERT_FALSE(claude_needs_you(L, 0));
  TEST_ASSERT_EQUAL_UINT8(0, claude_helpers(L));
}
void test_tool_groups() {
  const char *rd[] = {"Read", "Grep", "Glob", "WebSearch", "WebFetch", "ToolSearch", "LS"};
  for (const char *t : rd) TEST_ASSERT_EQUAL_MESSAGE((int)ClaudeState::READING, (int)after("PreToolUse", t), t);
  const char *cd[] = {"Edit", "Write", "NotebookEdit", "MultiEdit"};
  for (const char *t : cd) TEST_ASSERT_EQUAL_MESSAGE((int)ClaudeState::CODING, (int)after("PreToolUse", t), t);
  const char *rn[] = {"Bash", "PowerShell", "Task", "mcp__foo__bar", "", "read", "Reader"};
  for (const char *t : rn) TEST_ASSERT_EQUAL_MESSAGE((int)ClaudeState::RUNNING, (int)after("PreToolUse", t), t);
  TEST_ASSERT_EQUAL((int)ClaudeState::RUNNING, (int)after("PreToolUse", nullptr));
}
void test_listening_then_thinking() {
  ClaudeLogic L;
  claude_event(L, "UserPromptSubmit", "", "", "s1", false, 5000);
  TEST_ASSERT_EQUAL((int)ClaudeState::LISTENING, (int)claude_state(L, 5000));
  TEST_ASSERT_EQUAL((int)ClaudeState::LISTENING, (int)claude_state(L, 6199));
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)claude_state(L, 6200));
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)claude_state(L, 5000 + 10 * MIN));
}
void test_thinking_events() {
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)after("PostToolUse", "Bash"));
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)after("SubagentStop"));
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)after("SessionStart"));
}
void test_failed_then_thinking() {
  ClaudeLogic L;
  claude_event(L, "PostToolUseFailure", "Bash", "", "s1", false, 1000);
  TEST_ASSERT_EQUAL((int)ClaudeState::FAILED, (int)claude_state(L, 1000));
  TEST_ASSERT_EQUAL((int)ClaudeState::FAILED, (int)claude_state(L, 3999));
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)claude_state(L, 4000));
}
void test_interrupt_is_not_failed() {
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)after("PostToolUseFailure", "Bash", "", "s1", true));
}
void test_needs_you_triggers() {
  TEST_ASSERT_EQUAL((int)ClaudeState::NEEDS_YOU, (int)after("PermissionRequest", "Bash"));
  TEST_ASSERT_EQUAL((int)ClaudeState::NEEDS_YOU, (int)after("PreToolUse", "AskUserQuestion"));
  const char *nt[] = {"permission_prompt", "idle_prompt", "elicitation_dialog", "agent_needs_input"};
  for (const char *n : nt) TEST_ASSERT_EQUAL_MESSAGE((int)ClaudeState::NEEDS_YOU, (int)after("Notification", "", n), n);
  // andere Notification-Arten ändern nichts
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)after("Notification", "", "auth_success"));
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)after("Notification", "", ""));
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)after("Notification", "", nullptr));
}
void test_needs_you_flag_and_duration() {
  ClaudeLogic L;
  claude_event(L, "PermissionRequest", "Bash", "", "s1", false, 1000);
  TEST_ASSERT_TRUE(claude_needs_you(L, 1000));
  TEST_ASSERT_TRUE(claude_needs_you(L, 1000 + 20 * MIN));   // länger als 15 min, bis max. 30 min
  TEST_ASSERT_TRUE(claude_active(L, 1000 + 29 * MIN));
  TEST_ASSERT_FALSE(claude_needs_you(L, 1000 + 30 * MIN));
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)claude_state(L, 1000 + 30 * MIN));
}
void test_done_then_idle_then_off() {
  ClaudeLogic L;
  claude_event(L, "Stop", "", "", "s1", false, 10000);
  TEST_ASSERT_EQUAL((int)ClaudeState::DONE, (int)claude_state(L, 10000));
  TEST_ASSERT_EQUAL((int)ClaudeState::DONE, (int)claude_state(L, 17999));
  TEST_ASSERT_EQUAL((int)ClaudeState::IDLE, (int)claude_state(L, 18000));
  TEST_ASSERT_TRUE(claude_active(L, 10000 + 14 * MIN));
  TEST_ASSERT_EQUAL((int)ClaudeState::IDLE, (int)claude_state(L, 10000 + 15 * MIN - 1));
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)claude_state(L, 10000 + 15 * MIN));
  TEST_ASSERT_FALSE(claude_active(L, 10000 + 15 * MIN));
}
void test_timeout_15min() {
  ClaudeLogic L;
  claude_event(L, "PreToolUse", "Bash", "", "s1", false, 2000);
  TEST_ASSERT_EQUAL((int)ClaudeState::RUNNING, (int)claude_state(L, 2000 + 15 * MIN - 1));
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)claude_state(L, 2000 + 15 * MIN));
}
void test_session_end() {
  ClaudeLogic L;
  claude_event(L, "SubagentStart", "", "", "s1", false, 1000);
  claude_event(L, "PreToolUse", "Bash", "", "s1", false, 1100);
  claude_event(L, "SessionEnd", "", "", "s1", false, 1200);
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)claude_state(L, 1200));
  TEST_ASSERT_EQUAL_UINT8(0, claude_helpers(L));
}
void test_helpers_counter() {
  ClaudeLogic L;
  claude_event(L, "SubagentStart", "", "", "s1", false, 1000);
  claude_event(L, "SubagentStart", "", "", "s1", false, 1100);
  TEST_ASSERT_EQUAL_UINT8(2, claude_helpers(L));
  claude_event(L, "SubagentStop", "", "", "s1", false, 1200);
  TEST_ASSERT_EQUAL_UINT8(1, claude_helpers(L));
  claude_event(L, "SubagentStop", "", "", "s1", false, 1300);
  claude_event(L, "SubagentStop", "", "", "s1", false, 1400);   // nie < 0
  TEST_ASSERT_EQUAL_UINT8(0, claude_helpers(L));
  claude_event(L, "SubagentStart", "", "", "s1", false, 1500);
  TEST_ASSERT_EQUAL_UINT8(1, claude_helpers(L));
  TEST_ASSERT_EQUAL_UINT8(0, claude_helpers(L, 1500 + 15 * MIN) );   // nach Timeout wirkt der Zähler nicht mehr
}
void test_helpers_reset_after_timeout_on_next_event() {
  ClaudeLogic L;
  claude_event(L, "SubagentStart", "", "", "s1", false, 1000);
  claude_event(L, "SubagentStart", "", "", "s1", false, 1100);
  claude_event(L, "SessionStart", "", "", "s2", false, 1100 + 16 * MIN);
  TEST_ASSERT_EQUAL_UINT8(0, claude_helpers(L));
  claude_event(L, "SubagentStop", "", "", "s2", false, 1100 + 16 * MIN + 10);
  TEST_ASSERT_EQUAL_UINT8(0, claude_helpers(L));
}
void test_subagent_start_keeps_state() {
  ClaudeLogic L;
  claude_event(L, "PreToolUse", "Edit", "", "s1", false, 1000);
  claude_event(L, "SubagentStart", "", "", "s1", false, 1100);
  TEST_ASSERT_EQUAL((int)ClaudeState::CODING, (int)claude_state(L, 1100));
}
void test_newest_event_wins() {
  ClaudeLogic L;
  claude_event(L, "PreToolUse", "Read", "", "aaaa1111", false, 1000);
  claude_event(L, "PreToolUse", "Bash", "", "bbbb2222", false, 1100);
  TEST_ASSERT_EQUAL((int)ClaudeState::RUNNING, (int)claude_state(L, 1100));
}
void test_needs_you_has_priority_over_other_sessions() {
  ClaudeLogic L;
  claude_event(L, "PermissionRequest", "Bash", "", "aaaa1111", false, 1000);
  claude_event(L, "PreToolUse", "Bash", "", "bbbb2222", false, 1100);
  claude_event(L, "Stop", "", "", "bbbb2222", false, 1200);
  claude_event(L, "SessionEnd", "", "", "bbbb2222", false, 1250);
  TEST_ASSERT_EQUAL((int)ClaudeState::NEEDS_YOU, (int)claude_state(L, 1250));
  claude_event(L, "PostToolUse", "Bash", "", "aaaa1111", false, 1300);   // dieselbe Sitzung -> frei
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)claude_state(L, 1300));
  claude_event(L, "PreToolUse", "Bash", "", "bbbb2222", false, 1400);
  TEST_ASSERT_EQUAL((int)ClaudeState::RUNNING, (int)claude_state(L, 1400));
}
void test_needs_you_same_session_leaves_on_any_event() {
  ClaudeLogic L;
  claude_event(L, "Notification", "", "permission_prompt", "aaaa1111", false, 1000);
  claude_event(L, "Stop", "", "", "aaaa1111", false, 1100);
  TEST_ASSERT_EQUAL((int)ClaudeState::DONE, (int)claude_state(L, 1100));
}
void test_other_sessions_events_keep_alive() {
  ClaudeLogic L;
  claude_event(L, "PermissionRequest", "Bash", "", "aaaa1111", false, 1000);
  claude_event(L, "PreToolUse", "Bash", "", "bbbb2222", false, 1000 + 20 * MIN);
  TEST_ASSERT_EQUAL((int)ClaudeState::NEEDS_YOU, (int)claude_state(L, 1000 + 20 * MIN));
}
void test_sid_compares_first_8_chars() {
  ClaudeLogic L;
  claude_event(L, "PermissionRequest", "Bash", "", "abcdefgh-LONGER", false, 1000);
  claude_event(L, "Stop", "", "", "abcdefgh", false, 1100);
  TEST_ASSERT_EQUAL((int)ClaudeState::DONE, (int)claude_state(L, 1100));
}
void test_unknown_event_ignored() {
  ClaudeLogic L;
  claude_event(L, "Bogus", "", "", "s1", false, 1000);
  claude_event(L, nullptr, nullptr, nullptr, nullptr, false, 1000);
  TEST_ASSERT_EQUAL((int)ClaudeState::OFF, (int)claude_state(L, 1000));
}
void test_wraparound() {
  ClaudeLogic L;
  const uint32_t t = 0xFFFFFF00u;
  claude_event(L, "UserPromptSubmit", "", "", "s1", false, t);
  TEST_ASSERT_EQUAL((int)ClaudeState::LISTENING, (int)claude_state(L, t + 100));
  TEST_ASSERT_EQUAL((int)ClaudeState::THINKING, (int)claude_state(L, t + 2000));
}
void test_state_names() {
  TEST_ASSERT_EQUAL_STRING("off", claude_state_name(ClaudeState::OFF));
  TEST_ASSERT_EQUAL_STRING("idle", claude_state_name(ClaudeState::IDLE));
  TEST_ASSERT_EQUAL_STRING("listening", claude_state_name(ClaudeState::LISTENING));
  TEST_ASSERT_EQUAL_STRING("thinking", claude_state_name(ClaudeState::THINKING));
  TEST_ASSERT_EQUAL_STRING("reading", claude_state_name(ClaudeState::READING));
  TEST_ASSERT_EQUAL_STRING("coding", claude_state_name(ClaudeState::CODING));
  TEST_ASSERT_EQUAL_STRING("running", claude_state_name(ClaudeState::RUNNING));
  TEST_ASSERT_EQUAL_STRING("failed", claude_state_name(ClaudeState::FAILED));
  TEST_ASSERT_EQUAL_STRING("needs_you", claude_state_name(ClaudeState::NEEDS_YOU));
  TEST_ASSERT_EQUAL_STRING("done", claude_state_name(ClaudeState::DONE));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_initial_is_off);
  RUN_TEST(test_tool_groups);
  RUN_TEST(test_listening_then_thinking);
  RUN_TEST(test_thinking_events);
  RUN_TEST(test_failed_then_thinking);
  RUN_TEST(test_interrupt_is_not_failed);
  RUN_TEST(test_needs_you_triggers);
  RUN_TEST(test_needs_you_flag_and_duration);
  RUN_TEST(test_done_then_idle_then_off);
  RUN_TEST(test_timeout_15min);
  RUN_TEST(test_session_end);
  RUN_TEST(test_helpers_counter);
  RUN_TEST(test_helpers_reset_after_timeout_on_next_event);
  RUN_TEST(test_subagent_start_keeps_state);
  RUN_TEST(test_newest_event_wins);
  RUN_TEST(test_needs_you_has_priority_over_other_sessions);
  RUN_TEST(test_needs_you_same_session_leaves_on_any_event);
  RUN_TEST(test_other_sessions_events_keep_alive);
  RUN_TEST(test_sid_compares_first_8_chars);
  RUN_TEST(test_unknown_event_ignored);
  RUN_TEST(test_wraparound);
  RUN_TEST(test_state_names);
  return UNITY_END();
}
