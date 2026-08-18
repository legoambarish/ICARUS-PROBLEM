#ifndef DRIVERS_ADC_DRIVER_H
#define DRIVERS_ADC_DRIVER_H

#include <stdint.h>

typedef struct {
    int32_t (*read)(void);
} SensorDriver;

struct AppState;

SensorDriver *adc_sensor_driver(void);
void adc_driver_bind_state(struct AppState *state);
double adc_scale_voltage(int32_t voltage_raw);
int32_t adc_read_impl(void);

#endif
