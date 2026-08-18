#ifndef DRIVERS_TELEMETRY_INGEST_H
#define DRIVERS_TELEMETRY_INGEST_H

#include "flight/state.h"

void fsw_tm_push(AppState *state, const TelemetryFrame *frame);

#endif
