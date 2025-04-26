#include "shell_io.h"

static FILE *g_stdin = NULL; 
void shell_io_set_stdin(FILE *fp) { g_stdin = fp; }
FILE *shell_io_get_stdin(void)   { return g_stdin; }