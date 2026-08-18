#ifndef FLIGHT_FAULT_H
#define FLIGHT_FAULT_H

#include "flight/state.h"

void flight_fault_init(AppState *state);
void flight_fault_check_dispatch(void);
void fault_check_invoke(AppState *state);

#endif
