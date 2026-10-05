#ifndef MSPQ_TEST_PATTERN_H
#define MSPQ_TEST_PATTERN_H

#include "mspq1tmn1001qnx.h"

/* Caller provides a contiguous MSPQ_EPD_FRAME_BYTES buffer (105984 bytes). */
void mspq_make_test_pattern(uint8_t frame[MSPQ_EPD_FRAME_BYTES]);

#endif
