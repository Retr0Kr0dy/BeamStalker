#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

typedef enum { JOB_RUNNING, JOB_DONE, JOB_KILLED } job_state_t;

typedef struct bg_job {
    int          id;
    TaskHandle_t th;
    char        *cmd;
    job_state_t  state;
    char        *out_buf;
    size_t       out_len;
    size_t       out_cap;

    struct bg_job *next;
} bg_job_t;

bg_job_t *bg_add(const char *cmd, TaskHandle_t th);
void      bg_mark_done(TaskHandle_t th);
void      bg_kill(int id);
void      bg_list(void);
bool      bg_wait_fg(int id);
