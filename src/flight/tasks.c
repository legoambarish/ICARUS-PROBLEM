#include "drivers/adc_driver.h"
#include "drivers/radio_udp.h"
#include "drivers/telemetry_ingest.h"
#include "drivers/temp_copy.h"
#include "drivers/thermal_sensor.h"
#include "flight/fault.h"
#include "flight/power.h"
#include "flight/tasks.h"
#include "middleware/telemetry_pipeline.h"
#include "rtos/scheduler.h"

void telemetry_ingest_task(AppState *state) {
    TelemetryFrame frame;

    frame.tick = state->tick;
    frame.sensor_x = state->adc_raw;
    frame.sensor_y = state->current_temperature;
    frame.sensor_z = state->logged_temperature;
    frame.timestamp_ns = (uint64_t) state->tick * 1000000000ULL;
    fsw_tm_push(state, &frame);
}

void adc_sample_task(AppState *state) {
    SensorDriver *driver = adc_sensor_driver();

    state->adc_raw = driver->read();
    state->adc_voltage_reported = adc_scale_voltage(state->adc_raw);
}

void power_update_task(AppState *state) {
    power_update(state);
}

void sensor_refresh_task(AppState *state) {
    sensor_buffer_refresh(state);
    temp_guarded_copy(state, state->thermal_buffer.current_raw);
}

void thermal_monitor_task(AppState *state) {
    thermal_monitor_update(state);
}

void fault_check_task(AppState *state) {
    fault_check_invoke(state);
}

void comms_downlink_task(AppState *state) {
    packet_prepare(state);
    radio_send_packet(state, &state->last_packet);
}

void command_processing_task(AppState *state) {
    state->command_pending = false;
}
