#include "cmd_baatsh.h"

#include "cmd_default.h"
#include "cmd_echo.h"
#include "cmd_grep.h"
#include "cmd_sh.h"
#include "cmd_jobs.h"
#include "cmd_sleep.h"
#include "cmd_uptime.h"
#include "cmd_clear.h"

void register_builtins(void){
    register_default_dispatch();
    register_echo();
    register_grep();
    register_sh();
    register_jobs();
    register_kill_fg();
    register_sleep();
    register_uptime();
    register_clear();
}
