#pragma once
#include <cstdint>

enum class DisplayMode { TEMPERATURE, HUMIDITY, LIGHT, MOTION };

constexpr DisplayMode navigate(DisplayMode mode, bool clockwise) {
    return static_cast<DisplayMode>((static_cast<unsigned>(mode) +
                                    (clockwise ? 1U : 3U)) % 4U);
}

// CLK is bit 1, DT bit 0. Count only complete quadrature cycles.
class EncoderDecoder {
    uint8_t previous_ = 3;
    int progress_ = 0;
public:
    void reset(uint8_t pins) { previous_ = pins; progress_ = 0; }
    int update(uint8_t pins) {
        static constexpr int8_t steps[16] = {
            0,-1,1,0, 1,0,0,-1, -1,0,0,1, 0,1,-1,0
        };
        if ((previous_ ^ pins) == 3) { progress_ = 0; }
        else { progress_ += steps[previous_ * 4 + pins]; }
        previous_ = pins;
        if (pins != 3) { return 0; }
        const int result = progress_ == 4 ? 1 : (progress_ == -4 ? -1 : 0);
        progress_ = 0;
        return result;
    }
};
