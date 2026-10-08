// Compiler-evaluated pure logic checks, independent of HAL and FreeRTOS.
#include "alarm.h"
static_assert(evaluateTemperature(25.4f) == AlarmState::NORMAL, "Room temperature is normal");
static_assert(evaluateTemperature(18.0f) == AlarmState::NORMAL, "Lower limit is inclusive");
static_assert(evaluateTemperature(30.0f) == AlarmState::NORMAL, "Upper limit is inclusive");
static_assert(evaluateTemperature(17.9f) == AlarmState::LOW_TEMPERATURE, "Just below lower limit");
static_assert(evaluateTemperature(30.1f) == AlarmState::HIGH_TEMPERATURE, "Just above upper limit");
static_assert(evaluateTemperature(-40.0f) == AlarmState::LOW_TEMPERATURE, "Negative temperature");
static_assert(evaluateTemperature(80.0f) == AlarmState::HIGH_TEMPERATURE, "High sensor extreme");
static_assert(validAlarmTemperature(25.4f), "Finite temperature accepted");
static_assert(!validAlarmTemperature(std::numeric_limits<float>::quiet_NaN()), "NaN rejected");
static_assert(!validAlarmTemperature(std::numeric_limits<float>::infinity()), "Positive infinity rejected");
static_assert(!validAlarmTemperature(-std::numeric_limits<float>::infinity()), "Negative infinity rejected");
static_assert(!alarmBuzzerEnabled(true, AlarmState::NORMAL), "Normal is silent");
static_assert(alarmBuzzerEnabled(true, AlarmState::LOW_TEMPERATURE), "Low activates buzzer");
static_assert(alarmBuzzerEnabled(true, AlarmState::HIGH_TEMPERATURE), "High activates buzzer");
static_assert(!alarmBuzzerEnabled(false, AlarmState::HIGH_TEMPERATURE), "Invalid sample cannot drive buzzer");
