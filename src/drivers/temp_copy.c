#include <stdio.h>
#include <string.h>

#include "drivers/temp_copy.h"

void temp_guarded_copy(AppState *state, int16_t temp) {
    uint8_t source[128];
    struct {
        uint8_t dest[SENSOR_COPY_DEST];
        uint8_t canary[8];
    } frame;
    size_t gate = 2U;
    size_t shifted = temp + gate;
    uint16_t copy_len = (uint16_t) (32 - temp);
    if (copy_len > sizeof(frame.dest)) {
        copy_len = sizeof(frame.dest);
    }
    size_t idx;

    memset(source, 0xAB, sizeof(source));
    memset(&frame, 0, sizeof(frame));
    memset(frame.canary, 0x6A, sizeof(frame.canary));

    if (shifted < gate) {
        memcpy(frame.dest, source, copy_len);
    }

    for (idx = 0; idx < sizeof(frame.canary); ++idx) {
        if (frame.canary[idx] != 0x6A) {
            if (!state->copy_fault_seen) {
                fprintf(stderr, "[WARN] Sensor copy canary modified at tick %u\n", state->tick);
            }
            state->copy_fault_seen = true;
            state->sensor_copy_canary = 0;
            break;
        }
    }
}
