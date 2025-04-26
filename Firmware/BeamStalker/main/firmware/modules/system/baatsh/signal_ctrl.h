#pragma once
#include <stdbool.h>

void signal_ctrl_init(void);
bool sig_abort(void);
bool sig_bg(void);
void sig_clear(void);