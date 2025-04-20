/* Bourne Again Another Time Shell

   Dumb remake of basic sh fuctions
*/
#include "esp_console.h"
#include "esp_timer.h"
#include "argtable3/argtable3.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>

#include "cmd_system.h"

static const char *TAG = "baatsh";

static FILE *custom_stdin_fp = NULL;

void set_custom_stdin(FILE *fp) {
    custom_stdin_fp = fp;
}

FILE *get_custom_stdin(void) {
    return custom_stdin_fp;
}
static int cmd_clear(int argc, char **argv) {
    printf("\033[2J\033[H");  // ANSI escape sequence to clear screen
    return 0;
}

static struct {
    struct arg_str *text;
    struct arg_end *end;
} echo_args;

static int cmd_echo(int argc, char **argv) {
    FILE *stdin_fp = get_custom_stdin();
    if (stdin_fp) {
        char buf[128];
        while (fgets(buf, sizeof(buf), stdin_fp)) {
            printf("%s", buf);
        }
        return 0;
    }

    int nerrors = arg_parse(argc, argv, (void**)&echo_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, echo_args.end, argv[0]);
        return 1;
    }

    for (int i = 0; i < echo_args.text->count; i++) {
        printf("%s ", echo_args.text->sval[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_whoami(int argc, char **argv) {
    printf("You are... the mighty embedded hacker.\n");
    return 0;
}

static struct {
    struct arg_int *ms;
    struct arg_end *end;
} sleep_args;

static int cmd_sleep(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void**)&sleep_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, sleep_args.end, argv[0]);
        return 1;
    }
    int ms = sleep_args.ms->count > 0 ? sleep_args.ms->ival[0] : 1000;
    usleep(ms * 1000);
    return 0;
}

static int cmd_uptime(int argc, char **argv) {
    int64_t us = esp_timer_get_time();
    printf("Uptime: %.2f seconds\n", us / 1000000.0);
    return 0;
}

static struct {
    struct arg_str *pattern;
    struct arg_end *end;
} grep_args;

static int cmd_grep(int argc, char **argv) {
    FILE *stdin_fp = get_custom_stdin();

    int nerrors = arg_parse(argc, argv, (void**)&grep_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, grep_args.end, argv[0]);
        return 1;
    }

    if (!grep_args.pattern->count) {
        printf("grep: missing pattern\n");
        return 1;
    }

    const char *pattern = grep_args.pattern->sval[0];

    if (stdin_fp) {
        char line[256];
        while (fgets(line, sizeof(line), stdin_fp)) {
            if (strstr(line, pattern)) {
                printf("%s", line);
            }
        }
    } else {
        printf("grep: no input\n");
        return 1;
    }

    return 0;
}

char *strtrim(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;

    return str;
}
char *expand_command_substitution(const char *input) {
    char *result = calloc(1, 1024);
    if (!result) return NULL;

    size_t result_len = 0;
    const char *p = input;

    while (*p) {
        if (*p == '$' && *(p + 1) == '(') {
            p += 2;
            const char *start = p;
            int depth = 1;

            while (*p && depth > 0) {
                if (*p == '(') depth++;
                else if (*p == ')') depth--;
                p++;
            }

            if (depth != 0) {
                ESP_LOGE("baatsh", "Unmatched $()");
                free(result);
                return NULL;
            }

            size_t inner_len = (p - 1) - start;
            char *inner_cmd_raw = strndup(start, inner_len);
            if (!inner_cmd_raw) {
                free(result);
                return NULL;
            }

            char *inner_cmd = expand_command_substitution(inner_cmd_raw);
            free(inner_cmd_raw);
            if (!inner_cmd) {
                free(result);
                return NULL;
            }

            char output_buf[512] = {0};
            FILE *mem = fmemopen(output_buf, sizeof(output_buf), "w");
            if (!mem) {
                free(result);
                free(inner_cmd);
                return NULL;
            }

            FILE *saved_stdout = stdout;
            stdout = mem;

            int ret = 0;
            esp_console_run(inner_cmd, &ret);

            fflush(stdout);
            stdout = saved_stdout;
            fclose(mem);
            free(inner_cmd);

            size_t outlen = strlen(output_buf);
            if (outlen > 0 && output_buf[outlen - 1] == '\n') {
                output_buf[outlen - 1] = '\0';
            }

            strncat(result, output_buf, 1023 - result_len);
            result_len = strlen(result);
        } else {
            size_t len = strlen(result);
            if (len < 1023) {
                result[len] = *p;
                result[len + 1] = '\0';
            }
            p++;
        }
    }

    return result;
}

int baatsh_run(const char *input_line) {
    if (!input_line || strlen(input_line) == 0) return 0;

    char *expanded = expand_command_substitution(input_line);
    if (!expanded) {
        ESP_LOGE(TAG, "Failed to expand command substitutions");
        return 1;
    }

    char *line = strdup(expanded);
    free(expanded);
    if (!line) return 1;

    int result = 0;
    char *cmd_chain = strtok(line, "&&");

    while (cmd_chain) {
        cmd_chain = strtrim(cmd_chain);

        char *pipe_pos = strchr(cmd_chain, '|');
        FILE *pipe_fp = NULL;
        char *output_buffer = NULL;

        if (pipe_pos) {
            *pipe_pos = '\0';
            char *left_cmd = strtrim(cmd_chain);
            char *right_cmd = strtrim(pipe_pos + 1);

            size_t bufsize = 1024;
            output_buffer = calloc(1, bufsize);
            if (!output_buffer) {
                free(line);
                return 1;
            }

            FILE *mem_out = fmemopen(output_buffer, bufsize, "w");
            if (!mem_out) {
                free(output_buffer);
                free(line);
                return 1;
            }

            FILE *saved_stdout = stdout;
            stdout = mem_out;

            int ret = 0;
            esp_console_run(left_cmd, &ret);

            fflush(stdout);
            stdout = saved_stdout;
            fclose(mem_out);

            if (ret != 0) {
                free(output_buffer);
                result = 1;
                break;
            }

            pipe_fp = fmemopen(output_buffer, bufsize, "r");
            set_custom_stdin(pipe_fp);
            ret = 0;
            esp_console_run(right_cmd, &ret);
            set_custom_stdin(NULL);
            fclose(pipe_fp);
            free(output_buffer);

            if (ret != 0) {
                result = 1;
                break;
            }

        } else {
            int ret = 0;
            esp_console_run(cmd_chain, &ret);
            if (ret != 0) {
                result = 1;
                break;
            }
        }

        cmd_chain = strtok(NULL, "&&");
    }

    free(line);
    return result;
}

static struct {
    struct arg_str *line;
    struct arg_end *end;
} baatsh_args;

static int cmd_baatsh(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void**)&baatsh_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, baatsh_args.end, argv[0]);
        return 1;
    }

    const char *cmd_str = baatsh_args.line->sval[0];
    return baatsh_run(cmd_str);
}

void register_baatsh(void) {
    const esp_console_cmd_t clear_cmd = {
        .command = "clear",
        .help = "Clear the terminal screen.",
        .hint = NULL,
        .func = &cmd_clear,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&clear_cmd));

    echo_args.text = arg_strn(NULL, NULL, "<text>", 0, 100, "Text to echo back");
    echo_args.end = arg_end(1);
    const esp_console_cmd_t echo_cmd = {
        .command = "echo",
        .help = "Echo back the input text.",
        .hint = NULL,
        .func = &cmd_echo,
        .argtable = &echo_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&echo_cmd));

    const esp_console_cmd_t whoami_cmd = {
        .command = "whoami",
        .help = "Identify yourself.",
        .hint = NULL,
        .func = &cmd_whoami
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&whoami_cmd));

    sleep_args.ms = arg_int0(NULL, NULL, "<ms>", "Milliseconds to sleep");
    sleep_args.end = arg_end(1);
    const esp_console_cmd_t sleep_cmd = {
        .command = "sleep",
        .help = "Sleep for given milliseconds.",
        .hint = NULL,
        .func = &cmd_sleep,
        .argtable = &sleep_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&sleep_cmd));

    const esp_console_cmd_t uptime_cmd = {
        .command = "uptime",
        .help = "Show how long the device has been running.",
        .hint = NULL,
        .func = &cmd_uptime
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&uptime_cmd));

    grep_args.pattern = arg_str1(NULL, NULL, "<pattern>", "Pattern to search for");
    grep_args.end = arg_end(1);

    const esp_console_cmd_t grep_cmd = {
        .command = "grep",
        .help = "Search input lines for a pattern (supports stdin)",
        .hint = NULL,
        .func = &cmd_grep,
        .argtable = &grep_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&grep_cmd));

    baatsh_args.line = arg_str1(NULL, NULL, "<cmd>", "Command string with && and > logic");
    baatsh_args.end = arg_end(1);
    const esp_console_cmd_t baatsh_cmd = {
        .command = "baatsh",
        .help = "Execute a mini shell command string (supports '&&' and '>')",
        .hint = NULL,
        .func = &cmd_baatsh,
        .argtable = &baatsh_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&baatsh_cmd));
}
