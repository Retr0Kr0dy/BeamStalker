#include "shell_dispatch.h"
#include "shell_io.h"
#include "esp_console.h"
#include "esp_log.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TAG "shell_dispatch"

typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
} strbuf_t;

static bool sb_init(strbuf_t *sb, size_t initial)
{
    sb->buf = calloc(1, initial);
    sb->cap = sb->buf ? initial : 0;
    sb->len = 0;
    return sb->buf != NULL;
}

static bool sb_append(strbuf_t *sb, const char *data, size_t n)
{
    if (sb->len + n + 1 > sb->cap) {
        size_t newcap = (sb->cap + n + 1) * 2;
        char *p = realloc(sb->buf, newcap);
        if (!p) return false;
        sb->buf = p;
        sb->cap = newcap;
    }
    memcpy(sb->buf + sb->len, data, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
    return true;
}

static char *strtrim(char *s)
{
    while (isspace((unsigned char)*s)) ++s;
    size_t len = strlen(s);
    while (len && isspace((unsigned char)s[len - 1])) --len;
    s[len] = '\0';
    return s;
}

static bool expand_subst(const char *in, strbuf_t *out)
{
    const char *p = in;
    while (*p) {
        if (*p == '$' && p[1] == '(') {
            p += 2;                    /* skip $( */
            const char *start = p;
            int depth = 1;
            while (*p && depth) {
                if (*p == '(') depth++;
                else if (*p == ')') depth--;
                p++;
            }
            if (depth) { ESP_LOGE(TAG, "unmatched $()"); return false; }
            size_t portion = (size_t)(p - 1 - start);
            char *inner = strndup(start, portion);
            if (!inner) return false;

            /* expand inner recursively first */
            strbuf_t rec = {0};
            if (!sb_init(&rec, 64) || !expand_subst(inner, &rec)) {
                free(inner); free(rec.buf); return false; }

            /* run the inner command */
            char tmp[1024] = {0};
            FILE *mem = fmemopen(tmp, sizeof(tmp), "w");
            if (!mem) { free(inner); free(rec.buf); return false; }
            FILE *saved = stdout;
            stdout = mem;
            int rc = 0; esp_console_run(rec.buf, &rc);
            fflush(stdout); stdout = saved; fclose(mem);

            free(inner); free(rec.buf);
            size_t tlen = strlen(tmp);
            if (tlen && tmp[tlen - 1] == '\n') --tlen;
            if (!sb_append(out, tmp, tlen)) return false;
        } else {
            if (!sb_append(out, p, 1)) return false;
            p++;
        }
    }
    return true;
}

static char *find_token(char *s, const char *tok)
{
    int depth = 0; bool sq = false, dq = false;
    size_t toklen = strlen(tok);
    for (char *p = s; *p; ++p) {
        if (*p == '\'' && !dq) sq = !sq;
        else if (*p == '"' && !sq) dq = !dq;
        else if (!sq && !dq) {
            if (*p == '(' && p > s && p[-1] == '$') depth++;
            else if (*p == ')') depth--;
            else if (depth == 0 && strncmp(p, tok, toklen) == 0) return p;
        }
    }
    return NULL;
}

static int exec_pipeline(char *left, char *right)
{
    int rc = 0;

    /* capture left command output */
    const size_t cap = 4096;
    char *buf = calloc(1, cap);
    if (!buf) return ESP_ERR_NO_MEM;

    FILE *mem_w = fmemopen(buf, cap, "w");
    FILE *saved_out = stdout; stdout = mem_w;
    esp_console_run(left, &rc);
    fflush(stdout); stdout = saved_out; fclose(mem_w);
    if (rc) { free(buf); return rc; }

    /* feed captured output as stdin for right */
    FILE *mem_r = fmemopen(buf, cap, "r");
    shell_io_set_stdin(mem_r);
    rc = 0; esp_console_run(right, &rc);
    shell_io_set_stdin(NULL);

    fclose(mem_r); free(buf);
    return rc;
}

int shell_dispatch(const char *line_in)
{
    if (!line_in || !*line_in) return 0;

    /* (1) Expand $() -------------------------------------------------- */
    strbuf_t exp = {0};
    if (!sb_init(&exp, 128) || !expand_subst(line_in, &exp)) {
        free(exp.buf); return 1; }

    char *work = exp.buf; int rc = 0;

    while (work && *work) {
        char *sep_ptr = find_token(work, "&&");
        const char *sep = "&&";
        if (!sep_ptr) { sep_ptr = find_token(work, ";"); sep = ";"; }
        if (sep_ptr) { *sep_ptr = '\0'; sep_ptr += strlen(sep); }

        char *cmd = strtrim(work);
        if (*cmd) {
            shell_io_set_stdin(NULL);                    /* reset for each atom */
            char *pipe = find_token(cmd, "|");
            if (pipe) { *pipe = '\0'; rc = exec_pipeline(strtrim(cmd), strtrim(pipe + 1)); }
            else       { rc = 0; esp_console_run(cmd, &rc); }
        }

        if (rc && strcmp(sep, "&&") == 0) break;       /* stop on first error */
        work = sep_ptr;
    }

    free(exp.buf);
    return rc;
}