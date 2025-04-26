#include "cmd_wifisniff.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

#include "wifi_sniffer.h"

static const char *TAG = "wifisniff_cmd";

static struct {
    struct arg_str *src;
    struct arg_str *dst;
    struct arg_str *ap;
    struct arg_str *fc;
    struct arg_str *channels;
    struct arg_int *time;
    struct arg_lit *verbose;
    struct arg_end *end;
} sniff_args;

static bool parse_mac(const char *str, uint8_t mac[6]) {
    return sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &mac[0], &mac[1], &mac[2],
                  &mac[3], &mac[4], &mac[5]) == 6;
}

static int parse_mac_list(const char *csv, uint8_t out[][6], int max) {
    int count = 0;
    char *copy = strdup(csv);
    char *token = strtok(copy, ",");
    while (token && count < max) {
        if (parse_mac(token, out[count])) count++;
        token = strtok(NULL, ",");
    }
    free(copy);
    return count;
}

static int parse_channel_list(const char *csv, uint8_t *out, int max) {
    int count = 0;
    char *copy = strdup(csv);
    char *token = strtok(copy, ",");
    while (token && count < max) {
        int ch = atoi(token);
        if (ch >= 1 && ch <= 13) {
            out[count++] = (uint8_t)ch;
        }
        token = strtok(NULL, ",");
    }
    free(copy);
    return count;
}

static int do_wifisniff_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void**)&sniff_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, sniff_args.end, argv[0]);
        return 1;
    }

    int duration = sniff_args.time->count ? sniff_args.time->ival[0] : 0;
    int verbose = sniff_args.verbose->count > 0;

    uint8_t src_list[10][6], dst_list[10][6], ap_list[10][6];
    int src_count = 0, dst_count = 0, ap_count = 0;

    uint8_t channel_list[13];
    int channel_count = 0;

    if (sniff_args.src->count)
        src_count = parse_mac_list(sniff_args.src->sval[0], src_list, 10);
    if (sniff_args.dst->count)
        dst_count = parse_mac_list(sniff_args.dst->sval[0], dst_list, 10);
    if (sniff_args.ap->count)
        ap_count = parse_mac_list(sniff_args.ap->sval[0], ap_list, 10);

    if (sniff_args.fc->count) {
        active_fc_filter = parse_fc_filters(sniff_args.fc->sval[0]);
    } else {
        memset(&active_fc_filter, 0, sizeof(active_fc_filter));
    }

    if (sniff_args.channels && sniff_args.channels->count)
        channel_count = parse_channel_list(sniff_args.channels->sval[0], channel_list, 13);

    filter_set_multi(
        src_count ? src_list : NULL, src_count,
        dst_count ? dst_list : NULL, dst_count,
        ap_count  ? ap_list  : NULL, ap_count
    );

    bool no_filter =
        src_count == 0 &&
        dst_count == 0 &&
        ap_count == 0 &&
        active_fc_filter.include_count == 0 &&
        active_fc_filter.exclude_count == 0;

    if (no_filter) {
        ESP_LOGW(TAG, "No filters set, displaying all packets.");
    }

    return sniff(duration, NULL, verbose, channel_list, channel_count);
}

void module_wifisniff(void)
{
    sniff_args.src      = arg_str0("s", "src", "<mac,...>", "Source MAC filter(s)");
    sniff_args.dst      = arg_str0("d", "dst", "<mac,...>", "Destination MAC filter(s)");
    sniff_args.ap       = arg_str0("a", "ap",  "<mac,...>", "AP/BSSID MAC filter(s)");
    sniff_args.fc       = arg_str0("f", "fc",  "<hex,...>", "Frame control type(s)");
    sniff_args.channels = arg_str0("c", "channel", "<ch,...>", "Channel(s) to use (default: 1–13)");
    sniff_args.time     = arg_int0("t", "time", "<s>", "Sniff duration (0 = infinite)");
    sniff_args.verbose  = arg_lit0("v", "verbose", "Verbose output");
    sniff_args.end      = arg_end(6);

    const esp_console_cmd_t cmd = {
        .command = "wifisniff",
        .help = "Sniff 802.11 packets with optional filters",
        .hint = NULL,
        .func = &do_wifisniff_cmd,
        .argtable = &sniff_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
