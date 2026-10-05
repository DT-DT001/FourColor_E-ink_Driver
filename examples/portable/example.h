#ifndef MSPQ_PORTABLE_EXAMPLE_H
#define MSPQ_PORTABLE_EXAMPLE_H

#include "mspq1tmn1001qnx.h"

/* Call after implementing board_epd_port.c. This demo uses 105984 bytes of RAM. */
mspq_status_t portable_example_show_test_pattern(void);

#endif
