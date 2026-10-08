#pragma once
#include "FreeRTOS.h"
#include "event_groups.h"

constexpr EventBits_t EVENT_ACTIVE = 1U << 0;
constexpr EventBits_t EVENT_MOTION = 1U << 1;
constexpr EventBits_t EVENT_ALARM = 1U << 2;
constexpr EventBits_t EVENT_MASK = EVENT_ACTIVE | EVENT_MOTION | EVENT_ALARM;
