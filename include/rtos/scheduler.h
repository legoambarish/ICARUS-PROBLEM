#ifndef RTOS_SCHEDULER_H
#define RTOS_SCHEDULER_H

#include "flight/state.h"

void scheduler_init(AppState *state);
void scheduler_run_tick(AppState *state);
TaskRecord *scheduler_find_task(AppState *state, const char *name);

#endif
