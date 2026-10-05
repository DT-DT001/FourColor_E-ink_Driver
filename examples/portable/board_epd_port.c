/* Porting template, NOT a working GPIO implementation. Fill in each TODO.
 * This template deliberately reports failure until board initialization exists.
 * Reference wiring: DIN=2, CLK=3, CS=4, RST=5, DC=6, BUSY=7 on ESP32-P4.
 */
#include "board_epd_port.h"

static bool board_ready;

static bool board_write(void *context, mspq_signal_t signal, bool level)
{
    (void)context;
    (void)signal;
    (void)level;
    /* TODO: map MSPQ_PIN_DIN/CLK/CS/RST/DC to GPIOs and set the output level.
     * Return true on success, even when writing LOW; false means an I/O error.
     */
    return false;
}

static bool board_read_busy(void *context, bool *level)
{
    (void)context;
    (void)level;
    /* TODO: write the RAW BUSY GPIO level to *level and return read success.
     * HIGH = ready; LOW = busy. Do not invert it to "is busy".
     */
    return false;
}

static void board_delay_us(void *context, uint32_t delay_us)
{
    (void)context;
    (void)delay_us;
    /* TODO: accurate microsecond delay, at least the requested duration. */
}

static void board_delay_ms(void *context, uint32_t delay_ms)
{
    (void)context;
    (void)delay_ms;
    /* TODO: millisecond delay. With an RTOS, yield CPU to other tasks;
     * every positive delay must wait at least one tick, not round to zero.
     */
}

static uint32_t board_millis(void *context)
{
    (void)context;
    /* TODO: return a continuously advancing millisecond clock (uint32_t).
     * The timer must also advance while delays/refresh waits are running.
     */
    return 0;
}

static const mspq_hal_t board_hal = {
    .context = NULL,
    .write = board_write,
    .read_busy = board_read_busy,
    .delay_us = board_delay_us,
    .delay_ms = board_delay_ms,
    .millis = board_millis,
};

bool board_epd_port_init(void)
{
    /* TODO: configure the actual GPIOs and timer, then set board_ready=true.
     * DIN/CLK/CS/RST/DC are outputs; BUSY is input (reference adapter: pull-up).
     * Initial outputs: CS=HIGH, RST=HIGH, CLK=LOW, DC=LOW, DIN=LOW.
     */
    board_ready = false;
    return board_ready;
}

const mspq_hal_t *board_epd_port_hal(void)
{
    return board_ready ? &board_hal : NULL;
}
