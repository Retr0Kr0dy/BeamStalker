#include "bg_jobs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static bg_job_t *jobs = NULL;
static int next_id = 1;

static bg_job_t **find_ptr(int id)
{
    for (bg_job_t **p = &jobs; *p; p = &(*p)->next)
        if ((*p)->id == id) return p;
    return NULL;
}

bg_job_t *bg_add(const char *cmd, TaskHandle_t th)
{
    bg_job_t *j = calloc(1, sizeof *j);
    j->id   = next_id++;
    j->cmd  = strdup(cmd);
    j->th   = th;
    j->state = JOB_RUNNING;
    j->next = jobs;
    jobs    = j;

    return j;
}

void bg_mark_done(TaskHandle_t th)
{
    if (!th) th = xTaskGetCurrentTaskHandle();
    for (bg_job_t *j = jobs; j; j = j->next)
        if (j->th == th) { j->state = JOB_DONE; break; }
}


void bg_kill(int id)
{
    bg_job_t **pp = find_ptr(id);
    if (!pp) { printf("kill: no job %d\n", id); return; }
    vTaskDelete((*pp)->th);
    (*pp)->state = JOB_KILLED;
}

static void prune_if_finished(bg_job_t **pp)
{
    bg_job_t *j = *pp;
    if (j->state == JOB_RUNNING) return;

   *pp = j->next;
    free(j->cmd);
    free(j->out_buf);
    free(j);
}

void bg_list(void)
{
    if (!jobs) { puts("(no jobs)"); return; }

    bg_job_t **pp = &jobs;
    while (*pp) {
        bg_job_t *j = *pp;
        printf("[%d] %-7s %s\n", j->id,
               j->state == JOB_RUNNING ? "RUNNING" :
               j->state == JOB_DONE    ? "DONE"    : "KILLED",
               j->cmd);

        if (j->state == JOB_RUNNING) pp = &j->next;
        else {
            *pp = j->next;
            free(j->cmd); free(j->out_buf); free(j);
        }
    }
}

bool bg_wait_fg(int id)
{
    bg_job_t **pp = find_ptr(id);
    if (!pp) { printf("fg: no job %d\n", id); return false; }
    bg_job_t *j = *pp;
    if (j->state != JOB_RUNNING) {
        printf("fg: job %d is not running\n", id);
        prune_if_finished(pp);
        return false;
    }

    while (j->state == JOB_RUNNING) vTaskDelay(20 / portTICK_PERIOD_MS);

    if (j->out_len) fwrite(j->out_buf, 1, j->out_len, stdout);
    prune_if_finished(pp);
    return true;
}
