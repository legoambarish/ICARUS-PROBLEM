#ifndef FLIGHT_STATE_H
#define FLIGHT_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <netinet/in.h>

#define TICKS_PER_ORBIT 150
#define TOTAL_ORBITS 15
#define TOTAL_TICKS (TICKS_PER_ORBIT * TOTAL_ORBITS)
#define QUEUE_SIZE 1024
#define ADC_SHIFT 4
#define MAX_TEMP 90
#define SENSOR_COPY_DEST 16
#define THERMAL_WRAP_INCREMENT 38
#define THERMAL_WRAP_INITIAL 950
#define TICK_DELAY_MS 50
#define TEMP_CRITICAL 85.0
#define VIB_CRITICAL 12.0

typedef enum {
    TASK_PRIO_HIGH = 0,
    TASK_PRIO_MEDIUM = 1,
    TASK_PRIO_LOW = 2
} TaskPriority;

typedef struct {
    uint32_t tick;
    int32_t sensor_x;
    int32_t sensor_y;
    int32_t sensor_z;
    uint64_t timestamp_ns;
} TelemetryFrame;

typedef struct {
    uint32_t tick;
    double battery_voltage;
    int32_t temperature_raw;
    uint16_t crc;
} WireTelemetry;

typedef struct {
    uint32_t tick;
    int16_t temperature_raw;
    double battery_voltage;
    uint16_t crc;
} TelemetryPacket;

typedef struct {
    int16_t current_raw;
    int16_t previous_raw;
    uint32_t last_tick;
} ThermalBuffer;

typedef struct {
    double angle;
    bool locked;
} SolarPanelConfig;

typedef struct __attribute__((packed)) {
    TelemetryFrame queue[QUEUE_SIZE];
    uint8_t reserved[sizeof(TelemetryFrame) - sizeof(void *)];
    void (*fault_check_fn)(void);
} SharedMemoryRegion;

struct AppState;

typedef struct {
    uint32_t runtime_ms;
} ThermalTaskState;

typedef struct {
    uint32_t runtime_ms;
} RefreshTaskState;

typedef struct {
    const char *name;
    uint16_t period;
    TaskPriority priority;
    uint16_t run_count;
    uint16_t runtime_ms;
    void (*task_fn)(struct AppState *state);
} TaskRecord;

typedef struct AppState {
    uint32_t tick;
    uint32_t telemetry_frames;
    uint32_t packets_sent;
    uint32_t orbit_index;
    bool safe_mode;
    bool therm_stale;
    bool thermal_wrapped;
    bool copy_fault_seen;
    bool trap_triggered;
    bool crc_bad_seen;
    bool scheduler_bias_flipped;
    bool command_pending;

    int32_t adc_raw;
    double adc_voltage_reported;
    double battery_voltage_true;
    double battery_voltage_reported;
    int16_t current_temperature;
    int16_t logged_temperature;
    int16_t sensor_copy_canary;
    ThermalBuffer thermal_buffer;
    TelemetryPacket last_packet;
    SharedMemoryRegion shared;
    TelemetryFrame *telemetry_cursor;
    ThermalTaskState thermal_state;
    RefreshTaskState refresh_state;
    TaskRecord *tasks;
    size_t task_count;
    FILE *ground_log;
    int udp_socket;
    struct sockaddr_in ground_addr;
    void (*fault_dispatch)(void);
    double target_heater_pwm;    // 0 to 100
    double target_wheel_rpm;     // 0 to 5000
    double actual_temperature;
    double actual_wheel_rpm;
    double vibration_amplitude;
} AppState;

#endif
