#ifndef MSPQ1TMN1001QNX_H
#define MSPQ1TMN1001QNX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MSPQ_EPD_WIDTH       768U
#define MSPQ_EPD_HEIGHT      552U
#define MSPQ_EPD_GATE_HEIGHT 600U
#define MSPQ_EPD_ROW_BYTES   (MSPQ_EPD_WIDTH / 4U)
#define MSPQ_EPD_FRAME_BYTES (MSPQ_EPD_ROW_BYTES * MSPQ_EPD_HEIGHT)

typedef enum {
    MSPQ_OK = 0,
    MSPQ_ERR_ARGUMENT,
    MSPQ_ERR_HAL,
    MSPQ_ERR_BUSY_TIMEOUT,
} mspq_status_t;

typedef enum {
    MSPQ_REVISION_A0 = 0,
    MSPQ_REVISION_A1 = 1,
} mspq_revision_t;

/* Native 2bpp codes, four pixels per byte, most-significant pixel first. */
typedef enum {
    MSPQ_COLOR_BLACK  = 0,
    MSPQ_COLOR_WHITE  = 1,
    MSPQ_COLOR_YELLOW = 2,
    MSPQ_COLOR_RED    = 3,
} mspq_color_t;

typedef enum {
    MSPQ_PIN_DIN,
    MSPQ_PIN_CLK,
    MSPQ_PIN_CS,
    MSPQ_PIN_RST,
    MSPQ_PIN_DC,
} mspq_signal_t;

/*
 * MCU-independent transport interface. Porting to another platform only
 * requires these five callbacks. BUSY is active low.
 */
typedef struct {
    void *context;
    bool (*write)(void *context, mspq_signal_t signal, bool level);
    bool (*read_busy)(void *context, bool *level);
    void (*delay_us)(void *context, uint32_t delay_us);
    void (*delay_ms)(void *context, uint32_t delay_ms);
    uint32_t (*millis)(void *context);
} mspq_hal_t;

typedef struct {
    mspq_revision_t revision;
    uint32_t ready_timeout_ms;
    uint32_t refresh_timeout_ms;
    bool sleep_after_refresh;
} mspq_epd_config_t;

typedef struct {
    mspq_hal_t hal;
    mspq_epd_config_t config;
    bool initialized;
} mspq_epd_t;

mspq_status_t mspq_epd_init(mspq_epd_t *epd, const mspq_hal_t *hal,
                            const mspq_epd_config_t *config);

/*
 * frame is top-to-bottom, left-to-right, 2bpp MSB first:
 * black=0, white=1, yellow=2, red=3. The driver owns no framebuffer memory.
 */
mspq_status_t mspq_epd_display(mspq_epd_t *epd, const uint8_t *frame,
                               size_t frame_bytes);

mspq_status_t mspq_epd_power_off(mspq_epd_t *epd);
mspq_status_t mspq_epd_sleep(mspq_epd_t *epd);

/* Exposed for diagnostics and host-side mapping tests. */
uint16_t mspq_epd_gate_row(mspq_revision_t revision, uint16_t logical_y);
const char *mspq_epd_revision_name(mspq_revision_t revision);
const char *mspq_epd_status_name(mspq_status_t status);

#ifdef __cplusplus
}
#endif

#endif
