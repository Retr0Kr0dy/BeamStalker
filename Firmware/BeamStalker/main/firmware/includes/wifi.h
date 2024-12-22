#ifndef WIFI_H
#define WIFI_H

#include <M5Cardputer.h>

#include "esp_wifi.h"
#include "freertos/timers.h"

#include "../menu.h"

#define CHANNEL 11
#define MAX_AP_NUM 20

int packet_count = 0;
TimerHandle_t pps_timer;

typedef struct {
    char name[32];
    uint8_t address[6];
} AP;

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3);
void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size);
void pps_timer_callback(TimerHandle_t xTimer);
void init_pps_timer();
void stop_pps_timer();
int start_wifi(wifi_mode_t mode, bool promiscious);
int stop_wifi();

AP* scan_wifi_ap(int *ap_count);
void generate_random_mac(uint8_t *mac_addr);
AP* getSelectedAPs(menu Menu, AP* ap_info_list, int* selected_count);
AP* select_wifi_menu(int *selected_ap_count);

#endif
