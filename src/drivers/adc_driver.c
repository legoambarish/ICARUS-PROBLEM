#include <math.h>

#include "drivers/adc_driver.h"
#include "flight/state.h"

static AppState *g_state;

static int32_t sensor_read_dispatch(void) {
    return adc_read_impl();
}

SensorDriver *adc_sensor_driver(void) {
    static SensorDriver driver = {
        sensor_read_dispatch
    };

    return &driver;
}

void adc_driver_bind_state(AppState *state) {
    g_state = state;
}

int32_t adc_read_impl(void) {
    double analog = g_state->battery_voltage_true;

    return (int32_t) (analog * 16.0) << ADC_SHIFT;
}

double adc_scale_voltage(int32_t voltage_raw) {
    return (double) (voltage_raw >> ADC_SHIFT) / 16.0;
}
