#include "flight/fault.h"

static AppState *g_fault_state;

void flight_fault_check_dispatch(void) {
    if (g_fault_state != NULL) {
        g_fault_state->command_pending = false;
    }
}

void flight_fault_init(AppState *state) {
    g_fault_state = state;
    state->shared.fault_check_fn = flight_fault_check_dispatch;
}

void fault_check_invoke(AppState *state) {
    (void) state;
    state->shared.fault_check_fn();
}
