#ifndef FLIGHT_TASKS_H
#define FLIGHT_TASKS_H

#include "flight/state.h"

void telemetry_ingest_task(AppState *state);
void adc_sample_task(AppState *state);
void power_update_task(AppState *state);
void sensor_refresh_task(AppState *state);
void thermal_monitor_task(AppState *state);
void fault_check_task(AppState *state);
void comms_downlink_task(AppState *state);
void command_processing_task(AppState *state);

#endif
