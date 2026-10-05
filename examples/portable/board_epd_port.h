#ifndef BOARD_EPD_PORT_H
#define BOARD_EPD_PORT_H

#include "mspq1tmn1001qnx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize five output pins, BUSY input, and a monotonic millisecond timer. */
bool board_epd_port_init(void);

/* The returned HAL and its context must remain valid throughout each refresh. */
const mspq_hal_t *board_epd_port_hal(void);

#ifdef __cplusplus
}
#endif

#endif
