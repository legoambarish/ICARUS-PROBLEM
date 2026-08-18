#ifndef DRIVERS_RADIO_UDP_H
#define DRIVERS_RADIO_UDP_H

#include "flight/state.h"

int radio_open(AppState *state);
int radio_send_packet(AppState *state, const TelemetryPacket *packet);
void radio_close(AppState *state);

#endif
