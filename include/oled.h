#pragma once

// Only DisplayTask may call these functions (including initialization).
bool oled_init();
bool oled_line(unsigned page, const char *text);
const char *oled_error();
bool oled_set_enabled(bool enabled);
