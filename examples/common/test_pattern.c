#include "test_pattern.h"

void mspq_make_test_pattern(uint8_t frame[MSPQ_EPD_FRAME_BYTES])
{
    const uint8_t bars[] = {0x00, 0x55, 0xaa, 0xff};
    for (size_t y = 0; y < MSPQ_EPD_HEIGHT; ++y) {
        for (size_t x = 0; x < MSPQ_EPD_ROW_BYTES; ++x) {
            /* Top: black/white/yellow/red bars. Bottom: 50% black/white dots. */
            frame[y * MSPQ_EPD_ROW_BYTES + x] = y < MSPQ_EPD_HEIGHT * 3U / 4U
                ? bars[x / (MSPQ_EPD_ROW_BYTES / 4U)]
                : ((y & 1U) != 0U ? 0x44 : 0x11);
        }
    }
}
