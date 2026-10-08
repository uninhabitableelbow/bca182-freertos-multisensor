#pragma once
#include <cstdint>

enum class SystemState { ACTIVE, INACTIVE };

// Unsigned subtraction handles a wrapping 32-bit timebase.
constexpr SystemState activityState(bool motion, uint32_t now,
                                    uint32_t last_motion, uint32_t timeout) {
    return motion || static_cast<uint32_t>(now - last_motion) < timeout ?
        SystemState::ACTIVE : SystemState::INACTIVE;
}
