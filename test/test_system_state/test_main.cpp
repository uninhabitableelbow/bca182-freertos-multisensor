#include <unity.h>
#include "system_state.h"

void setUp() {}
void tearDown() {}

void test_active_before_timeout() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::ACTIVE), static_cast<int>(activityState(false, 14999, 0, 15000)));
}

void test_active_at_timeout() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::INACTIVE), static_cast<int>(activityState(false, 15000, 0, 15000)));
}

void test_inactive_without_motion() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::INACTIVE), static_cast<int>(activityState(false, 16000, 0, 15000)));
}

void test_inactive_wakes_with_motion() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::ACTIVE), static_cast<int>(activityState(true, 16000, 0, 15000)));
}

void test_motion_wins_at_timeout() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::ACTIVE), static_cast<int>(activityState(true, 15000, 0, 15000)));
}

void test_new_motion_restarts_interval() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::ACTIVE), static_cast<int>(activityState(false, 20000, 10000, 15000)));
}

void test_timeout_after_new_motion() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::INACTIVE), static_cast<int>(activityState(false, 25000, 10000, 15000)));
}

void test_wrap_before_timeout() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::ACTIVE), static_cast<int>(activityState(false, 9, 0xfffffff0U, 30)));
}

void test_wrap_exact_timeout() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::INACTIVE), static_cast<int>(activityState(false, 14, 0xfffffff0U, 30)));
}

void test_startup_grace_period() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SystemState::ACTIVE), static_cast<int>(activityState(false, 0, 0, 15000)));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_active_before_timeout);
    RUN_TEST(test_active_at_timeout);
    RUN_TEST(test_inactive_without_motion);
    RUN_TEST(test_inactive_wakes_with_motion);
    RUN_TEST(test_motion_wins_at_timeout);
    RUN_TEST(test_new_motion_restarts_interval);
    RUN_TEST(test_timeout_after_new_motion);
    RUN_TEST(test_wrap_before_timeout);
    RUN_TEST(test_wrap_exact_timeout);
    RUN_TEST(test_startup_grace_period);
    return UNITY_END();
}

