#include "navigation.h"
#include <cassert>

int main() {
    auto mode = DisplayMode::TEMPERATURE;
    mode = navigate(mode, true); assert(mode == DisplayMode::HUMIDITY);
    mode = navigate(mode, true); assert(mode == DisplayMode::LIGHT);
    mode = navigate(mode, true); assert(mode == DisplayMode::MOTION);
    mode = navigate(mode, true); assert(mode == DisplayMode::TEMPERATURE);
    mode = navigate(mode, false); assert(mode == DisplayMode::MOTION);
    mode = navigate(mode, false); assert(mode == DisplayMode::LIGHT);
    mode = navigate(mode, false); assert(mode == DisplayMode::HUMIDITY);
    mode = navigate(mode, false); assert(mode == DisplayMode::TEMPERATURE);
    EncoderDecoder d;
    assert(d.update(1) == 0); assert(d.update(0) == 0);
    assert(d.update(2) == 0); assert(d.update(3) == 1);
    assert(d.update(2) == 0); assert(d.update(0) == 0);
    assert(d.update(1) == 0); assert(d.update(3) == -1);
    // Bounce retraces an edge; partial and invalid cycles must not navigate.
    assert(d.update(1) == 0); assert(d.update(3) == 0);
    assert(d.update(1) == 0); assert(d.update(0) == 0);
    assert(d.update(2) == 0); assert(d.update(3) == 1);
    assert(d.update(0) == 0); assert(d.update(3) == 0);
    assert(d.update(3) == 0);
}
