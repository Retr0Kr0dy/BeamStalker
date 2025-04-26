#include "cmd_sleep.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static struct {
    struct arg_int *ms;
    struct arg_end *end;
} sleep_args;

static int cmd_sleep(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void**)&sleep_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, sleep_args.end, argv[0]);
        return 1;
    }
    int ms = sleep_args.ms->count > 0 ? sleep_args.ms->ival[0] : 1000;
    usleep(ms * 1000);
    return 0;
}

void register_sleep(void) {
    sleep_args.ms = arg_int0(NULL, NULL, "<ms>", "Milliseconds to sleep");
    sleep_args.end = arg_end(1);
    const esp_console_cmd_t sleep_cmd = {
        .command = "sleep",
        .help = "Sleep for given milliseconds",
        .hint = NULL,
        .func = &cmd_sleep,
        .argtable = &sleep_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&sleep_cmd));
}