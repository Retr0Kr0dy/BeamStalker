#include "signal_ctrl.h"

#include <unistd.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static volatile bool g_abort = false;
static volatile bool g_bg    = false;

static void uart_sniffer(void *arg)
{
    uint8_t byte;
    for (;;) {
        int r = read(0, &byte, 1);
        if (r == 1) {
            if (byte == 0x03) g_abort = true;  // Ctrl+C
            if (byte == 0x1A) g_bg    = true;  // Ctrl+Z
        }
        vTaskDelay(pdMS_TO_TICKS(10));
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
