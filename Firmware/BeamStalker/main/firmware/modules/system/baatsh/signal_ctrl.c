#include "signal_ctrl.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static volatile bool g_abort = false;
static volatile bool g_bg    = false;

static void uart_sniffer(void *arg)
{
    const uart_port_t CONSOLE_UART = UART_NUM_0;
    uint8_t byte;

    for (;;) {
        if (uart_read_bytes(CONSOLE_UART, &byte, 1,
                            pdMS_TO_TICKS(10)) == 1) {
            if (byte == 0x03) g_abort = true;
            if (byte == 0x1A) g_bg    = true;
        }
    }
}

void signal_ctrl_init(void)
{
    xTaskCreatePinnedToCore(uart_sniffer,
                            "sigMon",
                            2048,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            NULL,
                            0);
}

bool sig_abort(void) { return g_abort; }
bool sig_bg(void)    { return g_bg;    }

void sig_clear(void)
{
    g_abort = false;
    g_bg    = false;
}
