#include "wifi.h"

#include "esp_event.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "time.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3)
{
    return 0;
}

void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size){
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false));
}

void pps_timer_callback(TimerHandle_t xTimer) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d packets/sec", packet_count);

    printf ("[PPS] %s\n", buffer);
    packet_count = 0;
}

void init_pps_timer() {
    pps_timer = xTimerCreate("PPS_Timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, pps_timer_callback);
    if (pps_timer == NULL) {
        printf("Failed to create timer\n");
    } else {
        xTimerStart(pps_timer, 0);
    }
}

void stop_pps_timer() {
    if (pps_timer != NULL) {
        xTimerStop(pps_timer, 0);
        xTimerDelete(pps_timer, 0);
        pps_timer = NULL;
    }
}

int start_wifi(wifi_mode_t mode, bool promiscious) {
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(promiscious));

    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(20));

    return 0;
}

int stop_wifi() {
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    return 0;
}

AP *scan_wifi_ap(int *ap_count_out) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = { .min = 100, .max = 300 }
        }
    };

    esp_wifi_scan_start(&scan_config, true);
    uint16_t ap_num = 0;
    esp_wifi_scan_get_ap_num(&ap_num);

    if (ap_num == 0) {
        *ap_count_out = 0;
        return NULL;
    }

    wifi_ap_record_t *records = calloc(ap_num, sizeof(wifi_ap_record_t));
    if (!records) {
        *ap_count_out = 0;
        return NULL;
    }

    esp_wifi_scan_get_ap_records(&ap_num, records);

    AP *result = calloc(ap_num, sizeof(AP));
    if (!result) {
        free(records);
        *ap_count_out = 0;
        return NULL;
    }

    for (int i = 0; i < ap_num; i++) {
        strncpy(result[i].name, (char *)records[i].ssid, sizeof(result[i].name) - 1);
        memcpy(result[i].address, records[i].bssid, 6);
        result[i].rssi = records[i].rssi;
        result[i].channel = records[i].primary;
        result[i].authmode = records[i].authmode;
    }

    free(records);
    *ap_count_out = ap_num;
    return result;
}

void generate_random_mac(uint8_t *mac_addr) {
    mac_addr[0] = 0x02;
    mac_addr[1] = 0x13;
    mac_addr[2] = 0x37;
    
    for (int i = 3; i < 6; i++) {
        mac_addr[i] = rand() % 256;
    }
}