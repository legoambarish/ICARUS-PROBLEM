#ifndef MIDDLEWARE_CRC16_H
#define MIDDLEWARE_CRC16_H

#include <stddef.h>
#include <stdint.h>

uint16_t crc16_ccitt(const uint8_t *data, size_t len);

#endif
