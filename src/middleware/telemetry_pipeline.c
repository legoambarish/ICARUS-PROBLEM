#include <string.h>

#include "middleware/crc16.h"
#include "middleware/telemetry_pipeline.h"

void packet_units_adjust(TelemetryPacket *packet) {
    packet->temperature_raw = (int16_t) (packet->temperature_raw * 10);
}

void packet_prepare(AppState *state) {
    TelemetryPacket *packet = &state->last_packet;

    packet->tick = state->tick;
    packet->temperature_raw = state->logged_temperature;
    packet->battery_voltage = state->battery_voltage_reported;
    packet_units_adjust(packet);
    packet->crc = crc16_ccitt((const uint8_t *) packet, offsetof(TelemetryPacket, crc));
}

WireTelemetry packet_to_wire(const TelemetryPacket *packet) {
    WireTelemetry wire;

    memset(&wire, 0, sizeof(wire));
    wire.tick = packet->tick;
    wire.temperature_raw = packet->temperature_raw;
    wire.battery_voltage = packet->battery_voltage;
    wire.crc = packet->crc;
    return wire;
}
