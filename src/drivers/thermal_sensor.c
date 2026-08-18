#include <math.h>

#include "drivers/thermal_sensor.h"

int16_t thermal_model_sample(uint32_t tick) {
    double phase = (2.0 * 3.14159265358979323846 * (double) tick) / (double) TICKS_PER_ORBIT;
    double value = 8.0 + 26.0 * sin(phase - 1.1);
    return (int16_t) value;
}

void sensor_buffer_refresh(AppState *state) {
    state->thermal_buffer.previous_raw = state->thermal_buffer.current_raw;
    state->thermal_buffer.current_raw = thermal_model_sample(state->tick);
    state->thermal_buffer.last_tick = state->tick;
    state->current_temperature = state->thermal_buffer.current_raw;
}

void thermal_monitor_update(AppState *state) {
    if (state->scheduler_bias_flipped) {
        state->logged_temperature = state->thermal_buffer.previous_raw;
        state->therm_stale = true;
    } else {
        state->logged_temperature = state->thermal_buffer.current_raw;
        state->therm_stale = false;
    }
}
