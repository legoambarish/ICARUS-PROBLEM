#include <stddef.h>
#include <string.h>

#include "flight/tasks.h"
#include "rtos/scheduler.h"



static int slot_open(const AppState *state, const TaskRecord *task) {
    return ((state->tick - 1U) % task->period) == 0U;
}

static int task_before(const AppState *state, const TaskRecord *lhs, const TaskRecord *rhs) {
    if (lhs->priority != rhs->priority) {
        return lhs->priority < rhs->priority;
    }
    if (lhs->run_count != rhs->run_count) {
        return lhs->run_count < rhs->run_count;
    }
    if (lhs->priority == TASK_PRIO_MEDIUM && rhs->priority == TASK_PRIO_MEDIUM) {
        const RefreshTaskState *refresh = &state->refresh_state;
        const ThermalTaskState *thermal = &state->thermal_state;
        if (refresh->runtime_ms < thermal->runtime_ms) {
            return strcmp(lhs->name, "Sensor Buffer Refresh") == 0;
        } else {
            return strcmp(lhs->name, "Thermal Monitor") == 0;
        }
    }
    return strcmp(lhs->name, rhs->name) < 0;
}

void scheduler_init(AppState *state) {
    static TaskRecord tasks[] = {
        { "Telemetry Ingest", 1, TASK_PRIO_HIGH, 0, 0, telemetry_ingest_task },
        { "ADC Sample", 1, TASK_PRIO_HIGH, 0, 0, adc_sample_task },
        { "Power Battery Update", 1, TASK_PRIO_HIGH, 0, 0, power_update_task },
        { "Sensor Buffer Refresh", 1, TASK_PRIO_MEDIUM, 0, 0, sensor_refresh_task },
        { "Thermal Monitor", 1, TASK_PRIO_MEDIUM, 0, 0, thermal_monitor_task },
        { "Fault Check", 5, TASK_PRIO_MEDIUM, 0, 0, fault_check_task },
        { "Comms Downlink", 1, TASK_PRIO_LOW, 0, 0, comms_downlink_task },
        { "Command Processing", 10, TASK_PRIO_LOW, 0, 0, command_processing_task }
    };

    state->tasks = tasks;
    state->task_count = sizeof(tasks) / sizeof(tasks[0]);
    state->thermal_state.runtime_ms = 950;
    state->refresh_state.runtime_ms = 0;
}

TaskRecord *scheduler_find_task(AppState *state, const char *name) {
    size_t idx;
    for (idx = 0; idx < state->task_count; ++idx) {
        if (strcmp(state->tasks[idx].name, name) == 0) {
            return &state->tasks[idx];
        }
    }
    return NULL;
}

void scheduler_run_tick(AppState *state) {
    size_t step;
    int used[8] = { 0 };

    state->thermal_state.runtime_ms += 38;
    state->refresh_state.runtime_ms += 38;
    if (state->refresh_state.runtime_ms >= state->thermal_state.runtime_ms) {
        state->scheduler_bias_flipped = true;
    }

    for (step = 0; step < state->task_count; ++step) {
        size_t pick = state->task_count;
        size_t idx;

        for (idx = 0; idx < state->task_count; ++idx) {
            if (used[idx] || !slot_open(state, &state->tasks[idx])) {
                continue;
            }
            if (pick == state->task_count || task_before(state, &state->tasks[idx], &state->tasks[pick])) {
                pick = idx;
            }
        }

        if (pick == state->task_count) {
            break;
        }

        used[pick] = 1;
        state->tasks[pick].task_fn(state);
        state->tasks[pick].run_count++;
    }
}
