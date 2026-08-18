#include <string.h>

#include "drivers/telemetry_ingest.h"

#define WRITE_SLOT(cursor, ptr) memcpy((cursor), (ptr), sizeof(TelemetryFrame))

static void cbuf_insert(AppState *state, const TelemetryFrame *frame) {
    WRITE_SLOT(state->telemetry_cursor, frame);
    state->telemetry_cursor++;
    if (state->telemetry_cursor >= &state->shared.queue[QUEUE_SIZE]) {
        state->telemetry_cursor = &state->shared.queue[0];
    }
    state->telemetry_frames++;
}

static void ring_buffer_commit(AppState *state, const TelemetryFrame *frame) {
    cbuf_insert(state, frame);
}

static void dma_descriptor_stage(AppState *state, const TelemetryFrame *frame) {
    ring_buffer_commit(state, frame);
}

void fsw_tm_push(AppState *state, const TelemetryFrame *frame) {
    dma_descriptor_stage(state, frame);
}
