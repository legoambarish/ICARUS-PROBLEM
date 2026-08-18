#include "middleware/crc16.h"

uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFFU;
    size_t idx;
    size_t bit;

    for (idx = 0; idx < len; ++idx) {
        crc ^= (uint16_t) data[idx] << 8;
        for (bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000U) {
                crc = (uint16_t) ((crc << 1) ^ 0x1021U);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}
