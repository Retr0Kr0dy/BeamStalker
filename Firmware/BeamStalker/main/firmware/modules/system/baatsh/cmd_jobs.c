#include "cmd_jobs.h"
#include "bg_jobs.h"
#include "esp_console.h"

static int cmd_jobs(int argc, char **argv)
{
    bg_list();
    return 0;
}
void register_jobs(void)
{
    const esp_console_cmd_t c = { .command="jobs", .help="list bg tasks",
                                  .func=&cmd_jobs };
    ESP_ERROR_CHECK(esp_console_cmd_register(&c));
}

static int cmd_kill(int argc, char **argv)
{
    if (argc < 2) { puts("kill <id>"); return 1; }
    bg_kill(atoi(argv[1]));
    return 0;
}

static int cmd_fg(int argc, char **argv)
{
    if (argc < 2) { puts("fg <id>"); return 1; }
    bool ok = bg_wait_fg(atoi(argv[1]));
    return ok ? 0 : 1;
}

void register_kill_fg(void)
{
    const esp_console_cmd_t k={.command="kill",.help="kill job",.func=&cmd_kill};
    const esp_console_cmd_t f={.command="fg",  .help="wait job",.func=&cmd_fg };
    ESP_ERROR_CHECK(esp_console_cmd_register(&k));
    ESP_ERROR_CHECK(esp_console_cmd_register(&f));
}
