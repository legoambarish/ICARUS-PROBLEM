#ifndef FLIGHT_SAFE_MODE_H
#define FLIGHT_SAFE_MODE_H

#include "flight/state.h"

void safe_mode_maybe_enter(AppState *state);
void solar_deploy_sequence(void);

#endif
