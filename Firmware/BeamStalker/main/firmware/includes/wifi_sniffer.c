#include "wifi_sniffer.h"
#include <string.h>
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "freertos/task.h"

uint16_t *selected_t_filter = NULL;
int selected_t_filter_count = 0;
int sniffer_verbose = 0;
ap_info_t sniff_ap_list[MAX_AP_COUNT];
uint8_t sniff_ap_count = 0;
volatile bool sniffer_active = false;
fc_filter_t active_fc_filter = {0};


static uint8_t src_filters[MAX_FILTERS][6], dst_filters[MAX_FILTERS][6], ap_filters[MAX_FILTERS][6];
static int src_filter_count = 0, dst_filter_count = 0, ap_filter_count = 0;

static int sniff_packet_count = 0;
static int64_t sniff_start_us = 0;
static int64_t last_print_us = 0;

bool check_ctrl_c() {
    uint8_t data;
    int len = uart_read_bytes(CONFIG_ESP_CONSOLE_UART_NUM, &data, 1, 0);
    return (len > 0 && data == 0x03);
}

static bool mac_equals(const uint8_t *a, const uint8_t *b) {
    return memcmp(a, b, 6) == 0;
}

static bool match_mac_list(const uint8_t *mac, uint8_t list[][6], int count) {
    for (int i = 0; i < count; i++) {
        if (mac_equals(mac, list[i])) return true;
    }
    return false;
}

void filter_set_multi(uint8_t src[][6], int s_count, uint8_t dst[][6], int d_count, uint8_t ap[][6], int a_count) {
    if (s_count > 0) memcpy(src_filters, src, s_count * 6);
    if (d_count > 0) memcpy(dst_filters, dst, d_count * 6);
    if (a_count > 0) memcpy(ap_filters,  ap,  a_count * 6);
    src_filter_count = s_count;
    dst_filter_count = d_count;
    ap_filter_count = a_count;
}

const char *get_frame_type_name(uint8_t type, uint8_t subtype) {
    static char buf[64];
    if (type == 0) { // Management frames
        switch (subtype) {
            case 0x00: return "Assoc Req";
            case 0x01: return "Assoc Resp";
            case 0x02: return "Reassoc Req";
            case 0x03: return "Reassoc Resp";
            case 0x04: return "Probe Req";
            case 0x05: return "Probe Resp";
            case 0x06: return "Timing Adv";
            case 0x07: return "Reserved";
            case 0x08: return "Beacon";
            case 0x09: return "ATIM";
            case 0x0A: return "Disassoc";
            case 0x0B: return "Auth";
            case 0x0C: return "Deauth";
            case 0x0D: return "Action";
            case 0x0E: return "Action No Ack";
            default:
                snprintf(buf, sizeof(buf), "Mgmt(0x%02X)", subtype);
                return buf;
        }
    } else if (type == 1) { // Control frames
        switch (subtype) {
            case 0x07: return "Control Wrapper";
            case 0x08: return "Block Ack Req";
            case 0x09: return "Block Ack";
            case 0x0A: return "PS Poll";
            case 0x0B: return "RTS";
            case 0x0C: return "CTS";
            case 0x0D: return "ACK";
            case 0x0E: return "CF-End";
            case 0x0F: return "CF-End + CF-Ack";
            default:
                snprintf(buf, sizeof(buf), "Ctrl(0x%02X)", subtype);
                return buf;
        }
    } else if (type == 2) { // Data frames
        switch (subtype) {
            case 0x00: return "Data";
            case 0x01: return "Data + CF-Ack";
            case 0x02: return "Data + CF-Poll";
            case 0x03: return "Data + CF-Ack + CF-Poll";
            case 0x04: return "Null Data";
            case 0x05: return "CF-Ack";
            case 0x06: return "CF-Poll";
            case 0x07: return "CF-Ack + CF-Poll";
            case 0x08: return "QoS Data";
            case 0x09: return "QoS Data + CF-Ack";
            case 0x0A: return "QoS Data + CF-Poll";
            case 0x0B: return "QoS Data + CF-Ack + CF-Poll";
            case 0x0C: return "QoS Null";
            case 0x0D: return "Reserved";
            case 0x0E: return "QoS CF-Poll (No Data)";
            case 0x0F: return "QoS CF-Ack + CF-Poll (No Data)";
            default:
                snprintf(buf, sizeof(buf), "Data(0x%02X)", subtype);
                return buf;
        }
    } else {
        snprintf(buf, sizeof(buf), "Unknown(%u,%u)", type, subtype);
        return buf;
    }
}

const char *get_flag_state(int set) {
    return set ? "Yes" : "No";
}

void print_mac(const char *label, const uint8_t mac[6]) {
    printf("  %-8s: %02X:%02X:%02X:%02X:%02X:%02X\n", label,
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#include "esp_log.h"
#include <ctype.h>

static void hexdump(const uint8_t *data, size_t len) {
    printf("╭────────── Raw 802.11 Frame (len: %-5zu bytes) ────────╮\n", len);
    for (size_t i = 0; i < len; i += 16) {
        printf("│ %04zx: ", i);

        // Hex bytes
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%02x ", data[i + j]);
            } else {
                printf("   ");
            }
        }

        // ASCII view
        printf("│ | ");
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                char c = data[i + j];
                printf("%c", isprint((int)c) ? c : '.');
            } else {
                printf(" ");
            }
        }
        printf(" |\n");
    }
    printf("╰───────────────────────────────────────────────────────╯\n");
}


static const char *yesno(bool x) { return x ? "Yes" : "No"; }

static void print_sleuth_report(const wifi_promiscuous_pkt_t *ppkt) {
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    uint16_t fc = hdr->frame_ctrl;
    uint8_t type = (fc & 0x000C) >> 2;
    uint8_t subtype = (fc & 0x00F0) >> 4;

    uint8_t to_ds = (fc >> 8) & 1;
    uint8_t from_ds = (fc >> 9) & 1;
    uint8_t retry = (fc >> 11) & 1;
    uint8_t protected = (fc >> 14) & 1;

    uint16_t seq_ctrl = hdr->sequence_ctrl;
    uint8_t frag_num = seq_ctrl & 0xF;
    uint16_t seq_num = (seq_ctrl >> 4);

    // Raw Hex Dump
    hexdump(ppkt->payload, ppkt->rx_ctrl.sig_len);

    // Header Summary
    printf("\nFrame Control:\n");
    printf("  Type/Subtype  : 0x%02x (%s)\n", subtype, get_frame_type_name(type, subtype));
    printf("  ToDS/FromDS   : %s / %s\n", yesno(to_ds), yesno(from_ds));
    printf("  Retry         : %s\n", yesno(retry));
    printf("  Protected     : %s\n", yesno(protected));

    // MAC addresses
    printf("\nAddresses:\n");
    printf("  Receiver Addr : %02X:%02X:%02X:%02X:%02X:%02X\n", hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5]);
    printf("  Transmitter   : %02X:%02X:%02X:%02X:%02X:%02X\n", hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
    printf("  BSSID         : %02X:%02X:%02X:%02X:%02X:%02X\n", hdr->addr3[0], hdr->addr3[1], hdr->addr3[2], hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]);

    printf("\n802.11 Sequencing:\n");
    printf("  Seq Number    : %u\n", seq_num);
    printf("  Frag Number   : %u\n", frag_num);

    printf("\nSignal:\n");
    printf("  RSSI          : %d dBm\n", ppkt->rx_ctrl.rssi);
    printf("  Channel       : %d\n", ppkt->rx_ctrl.channel);

    // Sleuth-style deductions
    printf("\nAnalysis:\n");
    if (type == 0 && subtype == 8) {
        printf("  ▸ This is a Beacon frame from AP %02X:%02X:%02X:%02X:%02X:%02X\n",
               hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
               hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]);
        printf("  ▸ Likely advertising its presence on the air\n");
        printf("  ▸ Retry=%s, Encryption=%s\n", yesno(retry), yesno(protected));
    } else if (type == 2) {
        printf("  ▸ This is a Data frame — typical user or device traffic\n");
        printf("  ▸ ToDS=%s / FromDS=%s → ", yesno(to_ds), yesno(from_ds));
        if (to_ds && !from_ds)
            printf("Sent to AP\n");
        else if (!to_ds && from_ds)
            printf("From AP to STA\n");
        else
            printf("Ad-hoc or WDS\n");
    } else {
        printf("  ▸ Uncommon subtype or management/control frame\n");
    }

    printf("────────────────────────────────────────────\n");
}

fc_filter_t parse_fc_filters(const char *csv) {
    fc_filter_t filter = {0};
    char *copy = strdup(csv);
    char *token = strtok(copy, ",");

    while (token && (filter.include_count < 16 || filter.exclude_count < 16)) {
        while (*token == ' ') token++;
        bool is_exclude = (*token == '!');

        if (is_exclude) token++;

        uint16_t val = (uint16_t)strtol(token, NULL, 0);
        if (is_exclude)
            filter.exclude[filter.exclude_count++] = val;
        else
            filter.include[filter.include_count++] = val;

        token = strtok(NULL, ",");
    }

    free(copy);
    return filter;
}

bool fc_passes_filter(uint16_t fc, const fc_filter_t *filter) {
    for (int i = 0; i < filter->exclude_count; i++) {
        if (fc == filter->exclude[i]) return false;
    }

    if (filter->include_count == 0) return true;

    for (int i = 0; i < filter->include_count; i++) {
        if (fc == filter->include[i]) return true;
    }

    return false;
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    if (!sniffer_active) return;

    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    uint16_t fc = hdr->frame_ctrl;

    bool match = true;

    if (src_filter_count && !match_mac_list(hdr->addr2, src_filters, src_filter_count)) match = false;
    if (dst_filter_count && !match_mac_list(hdr->addr1, dst_filters, dst_filter_count)) match = false;
    if (ap_filter_count  && !match_mac_list(hdr->addr3, ap_filters, ap_filter_count))   match = false;

    if (!fc_passes_filter(fc, &active_fc_filter)) match = false;

    if (match) {
        sniff_packet_count++;
        if (!sniffer_verbose) {
            printf("*");
        } else {
            int64_t now = esp_timer_get_time();
            if (now - last_print_us >= 200000) {
                last_print_us = now;
                print_sleuth_report(ppkt);
            }
        }
    }
}

void wifi_sniffer_set_channel(uint8_t ch) {
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
}

void wifi_sniffer_init(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

int sniff(int duration, uint16_t *type_filter, int verbose, uint8_t *channels, int channel_count) {
    selected_t_filter = type_filter;
    sniffer_verbose = verbose;

    sniff_packet_count = 0;
    sniff_start_us = esp_timer_get_time();

    sniffer_active = true;
    wifi_sniffer_init();

    bool run_forever = (duration == 0);
    int64_t end_us = esp_timer_get_time() + ((int64_t)duration * 1000000);

    if (run_forever) {
        printf("[INFO] No duration specified — running indefinitely. Press Ctrl+C to stop.\n");
    }

    int ch_index = 0;
    uint8_t channel = 1;

    while (run_forever || esp_timer_get_time() < end_us) {
        if (check_ctrl_c()) {
            printf("\n[CTRL+C] Sniffing aborted by user.\n");
            break;
        }

        if (channel_count > 0) {
            channel = channels[ch_index % channel_count];
            ch_index++;
        } else {
            channel = (channel % WIFI_CHANNEL_MAX) + 1;
        }

        wifi_sniffer_set_channel(channel);

        for (int i = 0; i < 10; i++) {
            if (check_ctrl_c()) {
                printf("\n[CTRL+C] Sniffing aborted by user.\n");
                goto sniff_exit;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    sniff_exit:
    sniffer_active = false;
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    int64_t end_time_us = esp_timer_get_time();
    float elapsed = (end_time_us - sniff_start_us) / 1000000.0f;
    float pps = (elapsed > 0.0f) ? (sniff_packet_count / elapsed) : 0;

    printf("\n[STATS] Sniff complete\n");
    printf("        Duration   : %.2f s\n", elapsed);
    printf("        Matched    : %d packets\n", sniff_packet_count);
    printf("        Rate       : %.2f packets/sec\n\n", pps);

    return 0;
}
