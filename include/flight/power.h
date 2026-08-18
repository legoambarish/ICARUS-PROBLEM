#ifndef FLIGHT_POWER_H
#define FLIGHT_POWER_H

#include "flight/state.h"

double solar_input_for_tick(uint32_t tick);
void power_update(AppState *state);

#endif
