#include "mspq1tmn1001qnx.h"

#include <string.h>

#define MSPQ_DEFAULT_READY_TIMEOUT_MS   10000U
#define MSPQ_DEFAULT_REFRESH_TIMEOUT_MS 180000U

static bool hal_valid(const mspq_hal_t *hal)
{
    return hal && hal->write && hal->read_busy && hal->delay_us &&
           hal->delay_ms && hal->millis;
}

static mspq_status_t pin_write(mspq_epd_t *epd, mspq_signal_t signal, bool level)
{
    return epd->hal.write(epd->hal.context, signal, level) ? MSPQ_OK : MSPQ_ERR_HAL;
}

static mspq_status_t send_byte(mspq_epd_t *epd, uint8_t value, bool data_mode)
{
    if (pin_write(epd, MSPQ_PIN_DC, data_mode) != MSPQ_OK ||
        pin_write(epd, MSPQ_PIN_CS, false) != MSPQ_OK) {
        return MSPQ_ERR_HAL;
    }

    for (unsigned bit = 0; bit < 8; ++bit) {
        if (pin_write(epd, MSPQ_PIN_DIN, (value & 0x80U) != 0U) != MSPQ_OK) {
            return MSPQ_ERR_HAL;
        }
        value <<= 1;
        if (pin_write(epd, MSPQ_PIN_CLK, true) != MSPQ_OK) {
            return MSPQ_ERR_HAL;
        }
        epd->hal.delay_us(epd->hal.context, 1);
        if (pin_write(epd, MSPQ_PIN_CLK, false) != MSPQ_OK) {
            return MSPQ_ERR_HAL;
        }
        epd->hal.delay_us(epd->hal.context, 1);
    }

    return pin_write(epd, MSPQ_PIN_CS, true);
}

static mspq_status_t command(mspq_epd_t *epd, uint8_t value)
{
    return send_byte(epd, value, false);
}

static mspq_status_t data(mspq_epd_t *epd, uint8_t value)
{
    return send_byte(epd, value, true);
}

static mspq_status_t wait_ready(mspq_epd_t *epd, uint32_t timeout_ms)
{
    const uint32_t start = epd->hal.millis(epd->hal.context);
    for (;;) {
        bool busy_level = true;
        if (!epd->hal.read_busy(epd->hal.context, &busy_level)) {
            return MSPQ_ERR_HAL;
        }
        if (busy_level) {
            return MSPQ_OK;
        }
        if ((uint32_t)(epd->hal.millis(epd->hal.context) - start) >= timeout_ms) {
            return MSPQ_ERR_BUSY_TIMEOUT;
        }
        epd->hal.delay_ms(epd->hal.context, 10);
    }
}

static mspq_status_t reset_panel(mspq_epd_t *epd)
{
    mspq_status_t status = pin_write(epd, MSPQ_PIN_RST, true);
    epd->hal.delay_ms(epd->hal.context, 20);
    if (status == MSPQ_OK) {
        status = pin_write(epd, MSPQ_PIN_RST, false);
    }
    epd->hal.delay_ms(epd->hal.context, 40);
    if (status == MSPQ_OK) {
        status = pin_write(epd, MSPQ_PIN_RST, true);
    }
    epd->hal.delay_ms(epd->hal.context, 50);
    return status;
}

static mspq_status_t set_window(mspq_epd_t *epd, uint16_t y0, uint16_t y1)
{
    const uint16_t x1 = MSPQ_EPD_WIDTH - 1U;
    const uint8_t bytes[] = {
        0x00, 0x00, (uint8_t)(x1 >> 8), (uint8_t)x1,
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1, 0x01,
    };

    mspq_status_t status = command(epd, 0x83);
    for (size_t i = 0; status == MSPQ_OK && i < sizeof(bytes); ++i) {
        status = data(epd, bytes[i]);
    }
    return status;
}

static uint8_t reverse_four_pixels(uint8_t value)
{
    return (uint8_t)(((value & 0x03U) << 6) |
                     ((value & 0x0cU) << 2) |
                     ((value & 0x30U) >> 2) |
                     ((value & 0xc0U) >> 6));
}

uint16_t mspq_epd_gate_row(mspq_revision_t revision, uint16_t logical_y)
{
    if (logical_y >= MSPQ_EPD_HEIGHT) {
        return UINT16_MAX;
    }
    if (revision == MSPQ_REVISION_A1) {
        return logical_y < 300U ? (uint16_t)(2U * logical_y)
                                : (uint16_t)(2U * logical_y - 599U);
    }
    return logical_y < 276U ? (uint16_t)(2U * logical_y)
                            : (uint16_t)(1103U - 2U * logical_y);
}

mspq_status_t mspq_epd_init(mspq_epd_t *epd, const mspq_hal_t *hal,
                            const mspq_epd_config_t *config)
{
    if (!epd || !hal_valid(hal) || !config ||
        (config->revision != MSPQ_REVISION_A0 && config->revision != MSPQ_REVISION_A1)) {
        return MSPQ_ERR_ARGUMENT;
    }

    memset(epd, 0, sizeof(*epd));
    epd->hal = *hal;
    epd->config = *config;
    if (epd->config.ready_timeout_ms == 0) {
        epd->config.ready_timeout_ms = MSPQ_DEFAULT_READY_TIMEOUT_MS;
    }
    if (epd->config.refresh_timeout_ms == 0) {
        epd->config.refresh_timeout_ms = MSPQ_DEFAULT_REFRESH_TIMEOUT_MS;
    }

    mspq_status_t status = pin_write(epd, MSPQ_PIN_CS, true);
    if (status == MSPQ_OK) status = pin_write(epd, MSPQ_PIN_CLK, false);
    if (status == MSPQ_OK) status = pin_write(epd, MSPQ_PIN_RST, true);
    if (status == MSPQ_OK) status = reset_panel(epd);
    if (status == MSPQ_OK) status = wait_ready(epd, epd->config.ready_timeout_ms);
    if (status != MSPQ_OK) {
        return status;
    }
    epd->hal.delay_ms(epd->hal.context, 30);

    status = command(epd, 0x00);
    if (status == MSPQ_OK) status = data(epd, 0x0b);
    if (status == MSPQ_OK) status = command(epd, 0x61);
    if (status == MSPQ_OK) status = data(epd, (uint8_t)(MSPQ_EPD_WIDTH >> 8));
    if (status == MSPQ_OK) status = data(epd, (uint8_t)MSPQ_EPD_WIDTH);
    if (status == MSPQ_OK) status = data(epd, (uint8_t)(MSPQ_EPD_GATE_HEIGHT >> 8));
    if (status == MSPQ_OK) status = data(epd, (uint8_t)MSPQ_EPD_GATE_HEIGHT);

    /* This panel's verified startup sequence issues PON twice. */
    for (unsigned pass = 0; status == MSPQ_OK && pass < 2; ++pass) {
        status = command(epd, 0x04);
        epd->hal.delay_ms(epd->hal.context, 2);
        if (status == MSPQ_OK) {
            status = wait_ready(epd, epd->config.ready_timeout_ms);
        }
    }

    epd->initialized = status == MSPQ_OK;
    return status;
}

mspq_status_t mspq_epd_display(mspq_epd_t *epd, const uint8_t *frame,
                               size_t frame_bytes)
{
    if (!epd || !epd->initialized || !frame || frame_bytes != MSPQ_EPD_FRAME_BYTES) {
        return MSPQ_ERR_ARGUMENT;
    }

    mspq_status_t status = MSPQ_OK;
    for (uint16_t y = 0; status == MSPQ_OK && y < MSPQ_EPD_HEIGHT; ++y) {
        const uint16_t gate_y = mspq_epd_gate_row(epd->config.revision, y);
        status = set_window(epd, gate_y, gate_y);
        if (status == MSPQ_OK) {
            status = command(epd, 0x10);
        }

        const uint8_t *row = frame + (MSPQ_EPD_HEIGHT - 1U - y) * MSPQ_EPD_ROW_BYTES;
        for (size_t x = 0; status == MSPQ_OK && x < MSPQ_EPD_ROW_BYTES; ++x) {
            status = data(epd, reverse_four_pixels(row[MSPQ_EPD_ROW_BYTES - 1U - x]));
        }
        if ((y & 7U) == 7U) {
            epd->hal.delay_ms(epd->hal.context, 1);
        }
    }

    if (status == MSPQ_OK) status = set_window(epd, 0, MSPQ_EPD_GATE_HEIGHT - 1U);
    if (status == MSPQ_OK) status = command(epd, 0x12);
    if (status == MSPQ_OK) status = data(epd, 0x01);
    epd->hal.delay_ms(epd->hal.context, 5);
    if (status == MSPQ_OK) {
        status = wait_ready(epd, epd->config.refresh_timeout_ms);
    }

    if (status == MSPQ_OK) {
        status = mspq_epd_power_off(epd);
    }
    if (status == MSPQ_OK && epd->config.sleep_after_refresh) {
        status = mspq_epd_sleep(epd);
    }
    return status;
}

mspq_status_t mspq_epd_power_off(mspq_epd_t *epd)
{
    if (!epd || !epd->initialized) {
        return MSPQ_ERR_ARGUMENT;
    }
    mspq_status_t status = command(epd, 0x02);
    if (status == MSPQ_OK) status = data(epd, 0x00);
    epd->hal.delay_ms(epd->hal.context, 2);
    if (status == MSPQ_OK) {
        status = wait_ready(epd, epd->config.ready_timeout_ms);
    }
    return status;
}

mspq_status_t mspq_epd_sleep(mspq_epd_t *epd)
{
    if (!epd || !epd->initialized) {
        return MSPQ_ERR_ARGUMENT;
    }
    mspq_status_t status = command(epd, 0x07);
    if (status == MSPQ_OK) status = data(epd, 0xa5);
    epd->hal.delay_ms(epd->hal.context, 200);
    if (status == MSPQ_OK) {
        epd->initialized = false;
    }
    return status;
}

const char *mspq_epd_revision_name(mspq_revision_t revision)
{
    return revision == MSPQ_REVISION_A0 ? "A0" :
           revision == MSPQ_REVISION_A1 ? "A1" : "unknown";
}

const char *mspq_epd_status_name(mspq_status_t status)
{
    switch (status) {
    case MSPQ_OK: return "ok";
    case MSPQ_ERR_ARGUMENT: return "invalid argument/state";
    case MSPQ_ERR_HAL: return "HAL error";
    case MSPQ_ERR_BUSY_TIMEOUT: return "BUSY timeout";
    default: return "unknown error";
    }
}
