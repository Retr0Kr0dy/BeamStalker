#ifndef SIGNAL_CTRL_H
#define SIGNAL_CTRL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void signal_ctrl_init(void);
bool sig_abort(void);
bool sig_bg(void);
void sig_clear(void);

#ifdef __cplusplus
}
#endif

#endif 