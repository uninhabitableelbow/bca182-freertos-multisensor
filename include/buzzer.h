#pragma once

// AlarmTask exclusively owns the buzzer and TIM4 channel 3 on PB8.
bool buzzer_init();
void buzzer_set(bool enabled);
