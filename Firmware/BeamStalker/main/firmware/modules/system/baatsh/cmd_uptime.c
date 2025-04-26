#include "cmd_uptime.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_timer.h"
#include <stdio.h>
#include <string.h>

static int cmd_uptime(int argc, char **argv) {
    int64_t us = esp_timer_get_time();
    printf("Uptime: %.2f seconds\n", us / 1000000.0);
    return 0;
}

void register_uptime(void) {
    const esp_console_cmd_t uptime_cmd = {
        .command = "uptime",
        .help = "Show how long the device has been running",
        .hint = NULL,
        .func = &cmd_uptime
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&uptime_cmd));
}