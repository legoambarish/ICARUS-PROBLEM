#ifndef DRIVERS_THERMAL_SENSOR_H
#define DRIVERS_THERMAL_SENSOR_H

#include <stdint.h>
#include "flight/state.h"

int16_t thermal_model_sample(uint32_t tick);
void sensor_buffer_refresh(AppState *state);
void thermal_monitor_update(AppState *state);

#endif
