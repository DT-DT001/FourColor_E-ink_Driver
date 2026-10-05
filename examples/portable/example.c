#include "example.h"

#include "board_epd_port.h"
#include "test_pattern.h"

static uint8_t frame[MSPQ_EPD_FRAME_BYTES];

mspq_status_t portable_example_show_test_pattern(void)
{
    if (!board_epd_port_init()) {
        return MSPQ_ERR_HAL;
    }
    mspq_make_test_pattern(frame);
    const mspq_epd_config_t config = {
        .revision = MSPQ_REVISION_A1,
        .ready_timeout_ms = 10000,
        .refresh_timeout_ms = 180000,
        .sleep_after_refresh = true,
    };
    mspq_epd_t panel;
    mspq_status_t status = mspq_epd_init(&panel, board_epd_port_hal(), &config);
    if (status == MSPQ_OK) {
        status = mspq_epd_display(&panel, frame, sizeof(frame));
    }
    return status;
}
