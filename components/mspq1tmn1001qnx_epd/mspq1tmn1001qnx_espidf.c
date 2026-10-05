#include "mspq1tmn1001qnx_espidf.h"

#include <string.h>

#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static gpio_num_t signal_pin(const mspq_espidf_bus_t *bus, mspq_signal_t signal)
{
    switch (signal) {
    case MSPQ_PIN_DIN: return bus->config.pin_din;
    case MSPQ_PIN_CLK: return bus->config.pin_clk;
    case MSPQ_PIN_CS:  return bus->config.pin_cs;
    case MSPQ_PIN_RST: return bus->config.pin_rst;
    case MSPQ_PIN_DC:  return bus->config.pin_dc;
    default:           return GPIO_NUM_NC;
    }
}

static bool idf_write(void *context, mspq_signal_t signal, bool level)
{
    mspq_espidf_bus_t *bus = context;
    const gpio_num_t pin = signal_pin(bus, signal);
    return pin != GPIO_NUM_NC && gpio_set_level(pin, level ? 1 : 0) == ESP_OK;
}

static bool idf_read_busy(void *context, bool *level)
{
    mspq_espidf_bus_t *bus = context;
    if (!level) {
        return false;
    }
    *level = gpio_get_level(bus->config.pin_busy) != 0;
    return true;
}

static void idf_delay_us(void *context, uint32_t delay_us)
{
    mspq_espidf_bus_t *bus = context;
    uint32_t effective = bus->config.half_clock_us;
    if (delay_us > effective) {
        effective = delay_us;
    }
    if (effective > 0) {
        esp_rom_delay_us(effective);
    }
}

static void idf_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    if (delay_ms == 0) {
        taskYIELD();
        return;
    }
    /* Sub-tick positive delays must not collapse to zero at 100 Hz. */
    TickType_t ticks = pdMS_TO_TICKS(delay_ms);
    vTaskDelay(ticks > 0 ? ticks : 1);
}

static uint32_t idf_millis(void *context)
{
    (void)context;
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

mspq_status_t mspq_espidf_bus_init(mspq_espidf_bus_t *bus,
                                   const mspq_espidf_config_t *config)
{
    if (!bus || !config) {
        return MSPQ_ERR_ARGUMENT;
    }

    memset(bus, 0, sizeof(*bus));
    bus->config = *config;

    const uint64_t output_mask = (1ULL << config->pin_din) |
                                 (1ULL << config->pin_clk) |
                                 (1ULL << config->pin_cs) |
                                 (1ULL << config->pin_rst) |
                                 (1ULL << config->pin_dc);
    const gpio_config_t outputs = {
        .pin_bit_mask = output_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    const gpio_config_t busy = {
        .pin_bit_mask = 1ULL << config->pin_busy,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&outputs) != ESP_OK || gpio_config(&busy) != ESP_OK) {
        return MSPQ_ERR_HAL;
    }

    bus->hal.context = bus;
    bus->hal.write = idf_write;
    bus->hal.read_busy = idf_read_busy;
    bus->hal.delay_us = idf_delay_us;
    bus->hal.delay_ms = idf_delay_ms;
    bus->hal.millis = idf_millis;
    bus->initialized = true;

    gpio_set_level(config->pin_cs, 1);
    gpio_set_level(config->pin_clk, 0);
    gpio_set_level(config->pin_rst, 1);
    gpio_set_level(config->pin_dc, 0);
    gpio_set_level(config->pin_din, 0);
    return MSPQ_OK;
}

void mspq_espidf_bus_deinit(mspq_espidf_bus_t *bus)
{
    if (!bus || !bus->initialized) {
        return;
    }
    gpio_reset_pin(bus->config.pin_din);
    gpio_reset_pin(bus->config.pin_clk);
    gpio_reset_pin(bus->config.pin_cs);
    gpio_reset_pin(bus->config.pin_rst);
    gpio_reset_pin(bus->config.pin_dc);
    gpio_reset_pin(bus->config.pin_busy);
    bus->initialized = false;
}

const mspq_hal_t *mspq_espidf_hal(mspq_espidf_bus_t *bus)
{
    return bus && bus->initialized ? &bus->hal : NULL;
}
