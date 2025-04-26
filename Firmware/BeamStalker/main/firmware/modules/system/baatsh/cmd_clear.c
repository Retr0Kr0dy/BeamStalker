#include "cmd_clear.h"
#include "esp_console.h"
#include "esp_err.h"
#include <stdio.h>

static int cmd_clear(int argc, char **argv)
{
    printf("\033[2J\033[H");
    return 0;
}

void register_clear(void)
{
    const esp_console_cmd_t cmd = {
        .command  = "clear",
        .help     = "Clear the terminal screen",
        .hint     = NULL,
        .func     = &cmd_clear,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
