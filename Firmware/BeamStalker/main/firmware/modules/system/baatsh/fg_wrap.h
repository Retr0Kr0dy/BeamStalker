#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
bool shell_run_fg(const char *cmd);
bool app_sig_abort(void);

#ifdef __cplusplus
}
#endif
