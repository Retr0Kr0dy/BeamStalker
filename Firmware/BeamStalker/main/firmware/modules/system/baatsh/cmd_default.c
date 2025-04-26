#include "fg_wrap.h"
#include "esp_console.h"

static int cmd_default(int argc, char **argv)
{
    shell_run_fg(argv[0]);
    return 0;
}

void register_default_dispatch(void)
{
    const esp_console_cmd_t def = {
        .command = "",
        .help    = NULL,
        .hint    = NULL,
        .func    = &cmd_default
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&def));
}
