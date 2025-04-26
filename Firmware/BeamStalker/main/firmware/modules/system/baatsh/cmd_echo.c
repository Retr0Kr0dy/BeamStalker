#include "cmd_echo.h"
#include "shell_io.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static struct {
    struct arg_lit *interpret;
    struct arg_str *text;
    struct arg_end *end;
} echo_args;

static void interpret_escapes(const char *in, char *out, size_t outcap) {
    size_t o = 0;
    for (size_t i = 0; in[i] && o + 1 < outcap; ++i) {
        if (in[i] == '\\' && in[i+1]) {
            char c = in[++i];
            switch (c) {
                case 'n': out[o++] = '\n'; break;
                case 't': out[o++] = '\t'; break;
                case 'r': out[o++] = '\r'; break;
                case '\\': out[o++] = '\\'; break;
                case '"': out[o++] = '"';  break;
                case '0': out[o++] = '\0'; break;
                default:
                    out[o++] = '\\';
                    if (o + 1 < outcap) out[o++] = c;
            }
        } else {
            out[o++] = in[i];
        }
    }
    out[o] = '\0';
}

static int cmd_echo(int argc, char **argv)
{
    FILE *in = shell_io_get_stdin();
    if (in) {
        char buf[128];
        while (fgets(buf, sizeof buf, in)) {
            fputs(buf, stdout);
        }
        return 0;
    }

    int nerrors = arg_parse(argc, argv, (void**)&echo_args);
    if (nerrors) {
        arg_print_errors(stderr, echo_args.end, argv[0]);
        return 1;
    }

    size_t total = 0;
    for (int i = 0; i < echo_args.text->count; i++) {
        total += strlen(echo_args.text->sval[i]) + 1;
    }
    char *joined = malloc(total + 1);
    if (!joined) return 1;
    joined[0] = '\0';

    for (int i = 0; i < echo_args.text->count; i++) {
        strcat(joined, echo_args.text->sval[i]);
        if (i + 1 < echo_args.text->count) {
            strcat(joined, " ");
        }
    }

    if (echo_args.interpret->count) {
        char *out = malloc(total + 1);
        if (!out) {
            free(joined);
            return 1;
        }
        interpret_escapes(joined, out, total + 1);
        fputs(out, stdout);
        free(out);
    } else {
        fputs(joined, stdout);
    }

    putchar('\n');
    free(joined);
    return 0;
}

void register_echo(void)
{
    echo_args.interpret = arg_lit0("e", NULL, "interpret backslash escapes");
    echo_args.text      = arg_strn(NULL, NULL, "<text>", 0, 100, "Text to echo");
    echo_args.end       = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command  = "echo",
        .help     = "Echo text (use -e to interpret \\-escapes)",
        .hint     = NULL,
        .func     = &cmd_echo,
        .argtable = &echo_args,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
