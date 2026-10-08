#include "system_state.h"

static_assert(activityState(false, 0, 0, 15000) == SystemState::ACTIVE, "Startup grace period");
static_assert(activityState(false, 14999, 0, 15000) == SystemState::ACTIVE, "Before timeout");
static_assert(activityState(false, 15000, 0, 15000) == SystemState::INACTIVE, "Exact timeout");
static_assert(activityState(false, 16000, 0, 15000) == SystemState::INACTIVE, "Past timeout");
static_assert(activityState(true, 16000, 0, 15000) == SystemState::ACTIVE, "Motion wakes system");
static_assert(activityState(true, 15000, 0, 15000) == SystemState::ACTIVE, "Motion wins at boundary");
static_assert(activityState(false, 20000, 10000, 15000) == SystemState::ACTIVE, "Refreshed motion time");
static_assert(activityState(false, 25000, 10000, 15000) == SystemState::INACTIVE, "Timeout after refresh");
static_assert(activityState(false, 9, 0xfffffff0U, 30) == SystemState::ACTIVE, "Wrap before timeout");
static_assert(activityState(false, 14, 0xfffffff0U, 30) == SystemState::INACTIVE, "Wrap exact timeout");
