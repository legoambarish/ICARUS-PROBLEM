#include <stdio.h>

#include "logging/log.h"

void log_tick_line(const AppState *state) {
    printf("TICK:%u | ORBIT:%u | TEMP:%d | VBAT:%.3f | SAFE:%d | THERM_STALE: %d\n",
           state->tick,
           state->orbit_index,
           state->logged_temperature,
           state->battery_voltage_reported,
           state->safe_mode ? 1 : 0,
           state->therm_stale ? 1 : 0);
    fflush(stdout);
}
