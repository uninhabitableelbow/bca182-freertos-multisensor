#include <unity.h>
#include "alarm.h"

void setUp() {}
void tearDown() {}

void test_below_lower_threshold() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::LOW_TEMPERATURE), static_cast<int>(evaluateTemperature(17.9f)));
}

void test_exact_lower_threshold() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::NORMAL), static_cast<int>(evaluateTemperature(18.0f)));
}

void test_normal_temperature() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::NORMAL), static_cast<int>(evaluateTemperature(25.4f)));
}

void test_exact_upper_threshold() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::NORMAL), static_cast<int>(evaluateTemperature(30.0f)));
}

void test_above_upper_threshold() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(AlarmState::HIGH_TEMPERATURE), static_cast<int>(evaluateTemperature(30.1f)));
}

void test_normal_silences_buzzer() {
    TEST_ASSERT_FALSE(alarmBuzzerEnabled(true, evaluateTemperature(25.4f)));
}

void test_low_activates_buzzer() {
    TEST_ASSERT_TRUE(alarmBuzzerEnabled(true, evaluateTemperature(-5.4f)));
}

void test_high_activates_buzzer() {
    TEST_ASSERT_TRUE(alarmBuzzerEnabled(true, evaluateTemperature(34.0f)));
}

void test_invalid_sample_silences_buzzer() {
    TEST_ASSERT_FALSE(alarmBuzzerEnabled(false, AlarmState::HIGH_TEMPERATURE));
}

void test_nan_rejected() {
    TEST_ASSERT_FALSE(validAlarmTemperature(std::numeric_limits<float>::quiet_NaN()));
}

void test_infinities_rejected() {
    TEST_ASSERT_FALSE(validAlarmTemperature(std::numeric_limits<float>::infinity()));
    TEST_ASSERT_FALSE(validAlarmTemperature(-std::numeric_limits<float>::infinity()));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_below_lower_threshold);
    RUN_TEST(test_exact_lower_threshold);
    RUN_TEST(test_normal_temperature);
    RUN_TEST(test_exact_upper_threshold);
    RUN_TEST(test_above_upper_threshold);
    RUN_TEST(test_normal_silences_buzzer);
    RUN_TEST(test_low_activates_buzzer);
    RUN_TEST(test_high_activates_buzzer);
    RUN_TEST(test_invalid_sample_silences_buzzer);
    RUN_TEST(test_nan_rejected);
    RUN_TEST(test_infinities_rejected);
    return UNITY_END();
}

