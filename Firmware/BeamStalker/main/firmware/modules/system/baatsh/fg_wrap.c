#include "fg_wrap.h"
#include "bg_jobs.h"
#include "signal_ctrl.h"
#include "shell_dispatch.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char         *cmd;
    TaskHandle_t  parent;
} fg_ctx_t;

static void fg_task(void *arg)
{
    fg_ctx_t *ctx = (fg_ctx_t *)arg;
    shell_dispatch(ctx->cmd);
    if (ctx->parent) {
        xTaskNotifyGive(ctx->parent);
    }
    free(ctx->cmd);
    free(ctx);
    vTaskDelete(NULL);
}

static volatile bool g_app_abort = false;

bool app_sig_abort(void) {
    if (g_app_abort) {
        g_app_abort = false;
        return true;
    }
    return false;
}

bool shell_run_fg(const char *line)
{
    sig_clear();
    g_app_abort = false;

    fg_ctx_t *ctx = malloc(sizeof *ctx);
    if (!ctx) { puts("OOM"); return false; }
    ctx->cmd    = strdup(line);
    ctx->parent = xTaskGetCurrentTaskHandle();
   
    TaskHandle_t child;
    if (xTaskCreate(fg_task, "fg", 4096, ctx,
                    tskIDLE_PRIORITY + 1, &child) != pdPASS) {
        puts("spawn error");
        free(ctx->cmd);
        free(ctx);
        return false;
    }

    bool abort_pending = false;

    while (1) {
        if (sig_abort()) {
            if (!abort_pending) {
                abort_pending   = true;
                g_app_abort     = true;
                sig_clear();
            } else {
                vTaskDelete(child);
                puts("^C (forced)");
                sig_clear();
                return false;
            }
        }

        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20))) {
            break;
        }

        if (sig_bg()) {
            size_t cap = 2048;
            char *buf   = calloc(1, cap);
            bg_job_t *job = bg_add(ctx->cmd, child);
            job->out_buf = buf;
            job->out_cap = cap;
            vTaskSuspend(child);
            sig_clear();   
            puts("");
            return true;
        }
    }

    sig_clear();
    g_app_abort = false;
    return true;
}
