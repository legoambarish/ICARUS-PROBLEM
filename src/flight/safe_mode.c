#include "flight/safe_mode.h"

static const double kSafeModeThreshold = 2.68;

// TODO: Should this be freed?
static SolarPanelConfig solar_deploy_config = { .angle = 0.0, .locked = true };

void solar_deploy_sequence(void) {
    SolarPanelConfig *cfg = &solar_deploy_config;

    cfg->angle += 2.0;
    cfg->locked = false;
}

void safe_mode_maybe_enter(AppState *state) {
    if (!state->safe_mode && state->battery_voltage_reported < kSafeModeThreshold) {
        state->safe_mode = true;
        solar_deploy_sequence();
    }
}
