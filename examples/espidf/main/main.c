#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mspq1tmn1001qnx_espidf.h"
#include "test_pattern.h"

static const char *TAG = "fourcolor_demo";
/* Test patterns need RAM; real artwork can instead be const data in flash. */
static uint8_t frame[MSPQ_EPD_FRAME_BYTES];

void app_main(void)
{
    mspq_espidf_bus_t bus = {0};
    const mspq_espidf_config_t bus_config = {
        .pin_din = 2, .pin_clk = 3, .pin_cs = 4,
        .pin_rst = 5, .pin_dc = 6, .pin_busy = 7,
        .half_clock_us = 1,
    };
    const mspq_epd_config_t panel_config = {
        .revision = MSPQ_REVISION_A1,
        .ready_timeout_ms = 10000,
        .refresh_timeout_ms = 180000,
        .sleep_after_refresh = true,
    };
    mspq_status_t status = mspq_espidf_bus_init(&bus, &bus_config);
    if (status != MSPQ_OK) {
        ESP_LOGE(TAG, "GPIO initialization failed: %s", mspq_epd_status_name(status));
        return;
    }
    mspq_make_test_pattern(frame);

    /* Retry only on failure; a successful pattern is displayed once and retained. */
    for (;;) {
        mspq_epd_t panel;
        status = mspq_epd_init(&panel, mspq_espidf_hal(&bus), &panel_config);
        if (status == MSPQ_OK) {
            status = mspq_epd_display(&panel, frame, sizeof(frame));
        }
        if (status == MSPQ_OK) {
            ESP_LOGI(TAG, "Test pattern refreshed; panel asleep. No periodic refresh.");
            break;
        }
        ESP_LOGE(TAG, "Refresh failed: %s; retry in 60 seconds", mspq_epd_status_name(status));
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
    mspq_espidf_bus_deinit(&bus);
}
