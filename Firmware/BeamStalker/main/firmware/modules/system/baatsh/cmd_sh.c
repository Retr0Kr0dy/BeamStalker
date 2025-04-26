#include "cmd_sh.h"
#include "shell_dispatch.h"
#include "bg_jobs.h"
#include "fg_wrap.h"

#include "esp_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static char *reconstruct_line(int argc, char **argv)
{
    size_t len = 0;
    for (int i = 1; i < argc; ++i) len += strlen(argv[i]) + 3;
    char *out = malloc(len + 1);
    if (!out) return NULL;
    out[0] = '\0';
    for (int i = 1; i < argc; ++i) {
        if (i > 1) strcat(out, " ");
        bool q = strchr(argv[i], ' ') || strchr(argv[i], '\t');
        if (q) strcat(out, "\"");
        strcat(out, argv[i]);
        if (q) strcat(out, "\"");
    }
    return out;
}

static void bg_task(void *arg)
{
    bg_job_t *job = (bg_job_t *)arg;

    FILE *saved = stdout;
    FILE *mem   = fmemopen(job->out_buf, job->out_cap, "w");
    stdout = mem;

    shell_dispatch(job->cmd);

    fflush(stdout);
    stdout = saved;
    fclose(mem);
    job->out_len = strlen(job->out_buf);

    bg_mark_done(NULL);
    vTaskDelete(NULL);
}


static int cmd_sh(int argc, char **argv)
{
    if (argc == 1) { puts("usage: sh <line> [&]"); return 0; }

    bool background = false;
    if (strcmp(argv[argc - 1], "&") == 0) { background = true; --argc; }

    char *line = reconstruct_line(argc, argv);
    if (!line) { perror("sh"); return 1; }

    if (!background) {                      
        bool ok = shell_run_fg(line);
        int rc = ok ? 0 : 1;
        free(line);
        return rc;
    }

    size_t cap = 2048;
    char  *buf = calloc(1, cap);
    if (!buf) { perror("sh"); free(line); return 1; }

    bg_job_t *job = bg_add(line, NULL);
    printf("[%d] ", job->id);
    job->out_buf = buf;
    job->out_cap = cap;
    job->out_len = 0;

    TaskHandle_t th;
    if (xTaskCreate(bg_task, "bg", 4096,
                    job, tskIDLE_PRIORITY, &th) != pdPASS) {
        perror("sh spawn");
        job->state = JOB_KILLED;
        return 1;
    }
    job->th = th;
    printf("%p\n", (void *)th);

    return 0;
}

void register_sh(void)
{
    const esp_console_cmd_t cmd = {
        .command = "sh",
        .help    = "Run Bourne-style line (|, &&, $, … ; '&' backgrounds)",
        .hint    = NULL,
        .func    = &cmd_sh,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
