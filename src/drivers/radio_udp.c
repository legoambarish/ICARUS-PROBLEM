#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "drivers/radio_udp.h"
#include "middleware/telemetry_pipeline.h"

int radio_open(AppState *state) {
    memset(&state->ground_addr, 0, sizeof(state->ground_addr));
    state->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    state->ground_addr.sin_family = AF_INET;
    state->ground_addr.sin_port = htons(9001);
    state->ground_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    return state->udp_socket;
}

int radio_send_packet(AppState *state, const TelemetryPacket *packet) {
    WireTelemetry wire = packet_to_wire(packet);

    state->packets_sent++;
    return (int) sendto(state->udp_socket,
                        &wire,
                        sizeof(wire),
                        0,
                        (struct sockaddr *) &state->ground_addr,
                        sizeof(state->ground_addr));
}

void radio_close(AppState *state) {
    if (state->udp_socket > 0) {
        close(state->udp_socket);
    }
}
