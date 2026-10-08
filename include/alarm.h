#pragma once
#include <limits>

enum class AlarmState { NORMAL, LOW_TEMPERATURE, HIGH_TEMPERATURE };

constexpr float low_temperature_limit = 18.0f;
constexpr float high_temperature_limit = 30.0f;

// Hardware-independent decision; caller must supply a valid measurement.
constexpr AlarmState evaluateTemperature(float temperature) {
    return temperature < low_temperature_limit ? AlarmState::LOW_TEMPERATURE :
        (temperature > high_temperature_limit ? AlarmState::HIGH_TEMPERATURE :
                                               AlarmState::NORMAL);
}

constexpr bool validAlarmTemperature(float temperature) {
    return temperature >= -std::numeric_limits<float>::max() &&
           temperature <= std::numeric_limits<float>::max();
}

constexpr bool alarmBuzzerEnabled(bool valid, AlarmState state) {
    return valid && state != AlarmState::NORMAL;
}
