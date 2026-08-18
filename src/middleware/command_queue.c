#include "flight/state.h"
#include "middleware/command_queue.h"

static AppState *g_state;

int command_queue_depth(void) {
    return 4;
}

void cmd_bind_state(struct AppState *state) {
    g_state = state;
}

void cmd_set_actuators(double temp_pwm, double wheel_rpm) {
    g_state->target_heater_pwm = temp_pwm;
    g_state->target_wheel_rpm = wheel_rpm;
}
