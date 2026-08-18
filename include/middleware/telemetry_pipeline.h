#ifndef MIDDLEWARE_TELEMETRY_PIPELINE_H
#define MIDDLEWARE_TELEMETRY_PIPELINE_H

#include "flight/state.h"

void packet_prepare(AppState *state);
void packet_units_adjust(TelemetryPacket *packet);
WireTelemetry packet_to_wire(const TelemetryPacket *packet);

#endif
