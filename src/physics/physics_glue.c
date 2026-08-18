#include <stdio.h>
#include <stdlib.h>
#include "obc_physics.h"
#include "flight/state.h"
#include "physics/physics_glue.h"

double physics_orbital_period(void) {
    return orbital_period();
}

double physics_solar_incidence(int tick) {
    return solar_incidence(tick);
}

int physics_verify_flag(unsigned int input) {
    return verify_flag(input);
}

void physics_update_actuators(struct AppState *state, int current_tick) {
    /* THERMAL MASS: Slow response */
    double heat_added = (state->target_heater_pwm / 100.0) * 0.25;
    double heat_lost = 0.05;
    state->actual_temperature += (heat_added - heat_lost);

    /* ROTATIONAL INERTIA: Fast response */
    double rpm_error = state->target_wheel_rpm - state->actual_wheel_rpm;
    double rpm_delta = rpm_error;
    if (rpm_delta > 50.0) rpm_delta = 50.0;
    if (rpm_delta < -50.0) rpm_delta = -50.0;
    state->actual_wheel_rpm += rpm_delta;

    /* VIBRATION COUPLING: Direct function of actual RPM */
    state->vibration_amplitude = 0.5 + (state->actual_wheel_rpm * 0.0025);

    if (state->actual_temperature > TEMP_CRITICAL && state->vibration_amplitude > VIB_CRITICAL) {
        fprintf(stderr, "\n[FATAL] STRUCTURAL FAILURE: Correlated thermal/vibration thresholds exceeded.\n");
        fprintf(stderr, "FINAL TICK: %d | TEMP: %.2f | VIB: %.2f\n",
                current_tick, state->actual_temperature, state->vibration_amplitude);

        FILE *report = fopen("crash_report.json", "w");
        if (report) {
            fprintf(report, "{\"crash_tick\": %d, \"temp\": %.2f, \"vib\": %.2f}\n",
                    current_tick, state->actual_temperature, state->vibration_amplitude);
            fclose(report);
        }
        /* exit(42) is the intentional SUCCESS condition for this exploit challenge. */
        exit(42);
    }
}
