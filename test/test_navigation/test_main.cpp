#include <unity.h>
#include "navigation.h"

void setUp() {}
void tearDown() {}

void test_forward_temperature() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::HUMIDITY), static_cast<int>(navigate(DisplayMode::TEMPERATURE, true)));
}

void test_forward_humidity() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::LIGHT), static_cast<int>(navigate(DisplayMode::HUMIDITY, true)));
}

void test_forward_light() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::MOTION), static_cast<int>(navigate(DisplayMode::LIGHT, true)));
}

void test_forward_motion() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::TEMPERATURE), static_cast<int>(navigate(DisplayMode::MOTION, true)));
}

void test_reverse_temperature() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::MOTION), static_cast<int>(navigate(DisplayMode::TEMPERATURE, false)));
}

void test_reverse_motion() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::LIGHT), static_cast<int>(navigate(DisplayMode::MOTION, false)));
}

void test_reverse_light() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::HUMIDITY), static_cast<int>(navigate(DisplayMode::LIGHT, false)));
}

void test_reverse_humidity() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(DisplayMode::TEMPERATURE), static_cast<int>(navigate(DisplayMode::HUMIDITY, false)));
}

void test_clockwise_complete_cycle() {
    EncoderDecoder d;
    TEST_ASSERT_EQUAL_INT(0, d.update(1));
    TEST_ASSERT_EQUAL_INT(0, d.update(0));
    TEST_ASSERT_EQUAL_INT(0, d.update(2));
    TEST_ASSERT_EQUAL_INT(1, d.update(3));
}

void test_counterclockwise_complete_cycle() {
    EncoderDecoder d;
    TEST_ASSERT_EQUAL_INT(0, d.update(2));
    TEST_ASSERT_EQUAL_INT(0, d.update(0));
    TEST_ASSERT_EQUAL_INT(0, d.update(1));
    TEST_ASSERT_EQUAL_INT(-1, d.update(3));
}

void test_bounce_does_not_navigate() {
    EncoderDecoder d;
    TEST_ASSERT_EQUAL_INT(0, d.update(1));
    TEST_ASSERT_EQUAL_INT(0, d.update(3));
    TEST_ASSERT_EQUAL_INT(0, d.update(3));
}

void test_invalid_transition_does_not_navigate() {
    EncoderDecoder d;
    TEST_ASSERT_EQUAL_INT(0, d.update(0));
    TEST_ASSERT_EQUAL_INT(0, d.update(3));
}

void test_rapid_repeated_cycles() {
    EncoderDecoder d;
    for (unsigned i = 0; i < 20; ++i) {
        TEST_ASSERT_EQUAL_INT(0, d.update(1));
        TEST_ASSERT_EQUAL_INT(0, d.update(0));
        TEST_ASSERT_EQUAL_INT(0, d.update(2));
        TEST_ASSERT_EQUAL_INT(1, d.update(3));
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_forward_temperature);
    RUN_TEST(test_forward_humidity);
    RUN_TEST(test_forward_light);
    RUN_TEST(test_forward_motion);
    RUN_TEST(test_reverse_temperature);
    RUN_TEST(test_reverse_motion);
    RUN_TEST(test_reverse_light);
    RUN_TEST(test_reverse_humidity);
    RUN_TEST(test_clockwise_complete_cycle);
    RUN_TEST(test_counterclockwise_complete_cycle);
    RUN_TEST(test_bounce_does_not_navigate);
    RUN_TEST(test_invalid_transition_does_not_navigate);
    RUN_TEST(test_rapid_repeated_cycles);
    return UNITY_END();
}

