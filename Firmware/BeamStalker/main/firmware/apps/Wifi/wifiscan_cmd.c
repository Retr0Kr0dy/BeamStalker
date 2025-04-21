#include "wifiscan_cmd.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "wifiscan_cmd";

static const char *get_auth_mode_str(wifi_auth_mode_t auth) {
    switch (auth) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        default: return "UNKNOWN";
    }
}

static int do_wifiscan_cmd(int argc, char **argv) {
    start_wifi(WIFI_MODE_STA, true);
    vTaskDelay(pdMS_TO_TICKS(100));

    printf("[WIFISCAN] Scanning...\n");

    int ap_count = 0;
    AP *ap_info_list = scan_wifi_ap(&ap_count);

    stop_wifi();
    vTaskDelay(pdMS_TO_TICKS(100));

    if (!ap_info_list || ap_count <= 0 || ap_count > 100) {
        printf("No valid access points found.\n");
        return 1;
    }

    printf("\n[WIFISCAN] Found %d AP%s:\n\n", ap_count, ap_count > 1 ? "s" : "");
    printf(" CH  RSSI  ENC        BSSID              SSID\n");
    printf("--------------------------------------------------\n");

    for (int i = 0; i < ap_count; i++) {
        const AP *ap = &ap_info_list[i];
        const char *auth_str = get_auth_mode_str(ap->authmode);

        // Check for obviously broken records
        if (ap->channel == 0 || ap->channel > 14 || ap->rssi == 0) continue;

        printf(" %2d  %4d  %-9s  %02X:%02X:%02X:%02X:%02X:%02X  %s\n",
               ap->channel,
               ap->rssi,
               auth_str,
               ap->address[0], ap->address[1], ap->address[2],
               ap->address[3], ap->address[4], ap->address[5],
               ap->name);
    }

    free(ap_info_list);
    return 0;
}

void module_wifiscan(void)
{
    const esp_console_cmd_t wifiscan_cmd = {
        .command = "wifiscan",
        .help = "Scan nearby Wi-Fi APs with RSSI, channel, and encryption info",
        .hint = NULL,
        .func = &do_wifiscan_cmd,
        .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&wifiscan_cmd));
}
