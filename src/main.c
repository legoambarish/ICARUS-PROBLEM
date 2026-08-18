#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "drivers/adc_driver.h"
#include "drivers/radio_udp.h"
#include "flight/fault.h"
#include "flight/state.h"
#include "logging/log.h"
#include "rtos/scheduler.h"

#include "obc_physics.h"
#include "physics/physics_glue.h"
#include "middleware/command_queue.h"

static AppState *g_state;

static void handle_segv(int signo) {
    (void) signo;
    fprintf(stderr, "[FATAL] Segmentation fault at tick %u\n", g_state ? g_state->tick : 0U);
    fflush(stderr);
    _Exit(139);
}

int main(void) {
    AppState state;

    memset(&state, 0, sizeof(state));
    state.telemetry_cursor = &state.shared.queue[0];
    state.sensor_copy_canary = 0x6A6A;
    state.battery_voltage_true = 6.35;
    g_state = &state;

    signal(SIGSEGV, handle_segv);
    radio_open(&state);
    adc_driver_bind_state(&state);
    flight_fault_init(&state);
    scheduler_init(&state);

    volatile int physics_link_check = physics_verify_flag(0);

    cmd_bind_state(&state);

    state.actual_temperature = 20.0;  /* Baseline ambient */
    state.actual_wheel_rpm = 0.0;
    state.vibration_amplitude = 0.5;  /* Baseline vibration */
    state.target_heater_pwm = 0.0;
    state.target_wheel_rpm = 0.0;

    for (state.tick = 1; state.tick <= TOTAL_TICKS; ++state.tick) {
        state.orbit_index = ((state.tick - 1U) / TICKS_PER_ORBIT) + 1U;
        scheduler_run_tick(&state);
        physics_update_actuators(&state, (int) state.tick);
        log_tick_line(&state);
        {
            struct timeval delay;

            delay.tv_sec = TICK_DELAY_MS / 1000;
            delay.tv_usec = (TICK_DELAY_MS % 1000) * 1000;
            select(0, NULL, NULL, NULL, &delay);
        }
    }

    radio_close(&state);
    return 0;
}
