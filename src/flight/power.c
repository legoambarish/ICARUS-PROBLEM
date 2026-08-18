#include <math.h>

#include "flight/power.h"
#include "flight/safe_mode.h"
#include "physics/physics_glue.h"

static const double kBaseInput = 1.54;
static const double kAmplitude = 0.77;
static const double kLoadBias = 1.58;
static const double kChargeGain = 0.0023;
static const double kDragLoss = 0.00056;

double solar_input_for_tick(uint32_t tick) {
    double phase = (2.0 * 3.14159265358979323846 * (double) tick) / (double) TICKS_PER_ORBIT;

    return kBaseInput + kAmplitude * sin(phase) + physics_solar_incidence((int) tick) * 0.05;
}

void power_update(AppState *state) {
    double delta = ((solar_input_for_tick(state->tick) - kLoadBias) * kChargeGain) - kDragLoss;

    state->battery_voltage_true += delta;
    state->battery_voltage_reported = state->adc_voltage_reported;
    safe_mode_maybe_enter(state);
}
