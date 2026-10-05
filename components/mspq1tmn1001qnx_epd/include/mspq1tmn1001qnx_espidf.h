#ifndef MSPQ1TMN1001QNX_ESPIDF_H
#define MSPQ1TMN1001QNX_ESPIDF_H

#include <stdint.h>

#include "driver/gpio.h"
#include "mspq1tmn1001qnx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pin_din;
    gpio_num_t pin_clk;
    gpio_num_t pin_cs;
    gpio_num_t pin_rst;
    gpio_num_t pin_dc;
    gpio_num_t pin_busy;
    uint32_t half_clock_us;
} mspq_espidf_config_t;

typedef struct {
    mspq_espidf_config_t config;
    mspq_hal_t hal;
    bool initialized;
} mspq_espidf_bus_t;

mspq_status_t mspq_espidf_bus_init(mspq_espidf_bus_t *bus,
                                   const mspq_espidf_config_t *config);
void mspq_espidf_bus_deinit(mspq_espidf_bus_t *bus);
const mspq_hal_t *mspq_espidf_hal(mspq_espidf_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif
