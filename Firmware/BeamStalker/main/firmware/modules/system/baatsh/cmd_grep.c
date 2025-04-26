#include "cmd_grep.h"
#include "shell_io.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include <stdio.h>
#include <string.h>
#include <regex.h>

static struct {
    struct arg_lit *extended;
    struct arg_str *pattern;
    struct arg_end *end;
} grep_args;

static int cmd_grep(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void **)&grep_args)) {
        arg_print_errors(stderr, grep_args.end, argv[0]);
        return 1;
    }

    if (!grep_args.pattern->count) {
        printf("grep: missing pattern\n");
        return 1;
    }
    const char *pat = grep_args.pattern->sval[0];
    FILE *in = shell_io_get_stdin();
    if (!in) {
        printf("grep: no input\n");
        return 1;
    }

    size_t len;
    if (grep_args.extended->count) {
        regex_t re;
        if (regcomp(&re, pat, REG_EXTENDED | REG_NOSUB)) {
            char errbuf[128];
            regerror(REG_ESPACE, &re, errbuf, sizeof(errbuf));
            fprintf(stderr, "grep: invalid regex\n");
            return 1;
        }
        char line[256];
        while (fgets(line, sizeof line, in)) {
            len = strlen(line);
            if (len && line[len - 1] == '\n') line[--len] = '\0';
            if (regexec(&re, line, 0, NULL, 0) == 0) {
                fputs(line, stdout);
                putchar('\n');
            }
        }
        regfree(&re);
    } else {
        char line[256];
        while (fgets(line, sizeof line, in)) {
            len = strlen(line);
            if (len && line[len - 1] == '\n') line[--len] = '\0';
            if (strstr(line, pat)) {
                fputs(line, stdout);
                putchar('\n');
            }
        }
    }

    return 0;
}

void register_grep(void)
{
    grep_args.extended = arg_lit0("E", "extended", "use extended regex");
    grep_args.pattern  = arg_str1(NULL, NULL, "<pattern>", "pattern");
    grep_args.end      = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command  = "grep",
        .help     = "search stdin for substring or regex",
        .hint     = NULL,
        .func     = &cmd_grep,
        .argtable = &grep_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
