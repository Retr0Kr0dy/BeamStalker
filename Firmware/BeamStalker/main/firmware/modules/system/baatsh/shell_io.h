#pragma once
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

void shell_io_set_stdin(FILE *fp);

FILE *shell_io_get_stdin(void);

#ifdef __cplusplus
}
#endif
