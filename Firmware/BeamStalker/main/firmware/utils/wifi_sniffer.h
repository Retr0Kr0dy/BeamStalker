#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H


#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_AP_COUNT           50
#define MAX_CLIENTS_PER_AP     20
#define MAX_FILTERS            10
#define WIFI_CHANNEL_MAX       13

extern uint16_t *selected_t_filter;
extern int selected_t_filter_count;
extern int sniffer_verbose;

typedef struct {
    uint8_t mac[6];
} mac_addr_t;

typedef struct {
    mac_addr_t ap_mac;
    mac_addr_t clients[MAX_CLIENTS_PER_AP];
    uint8_t client_count;
} ap_info_t;

extern ap_info_t sniff_ap_list[MAX_AP_COUNT];
extern uint8_t sniff_ap_count;

typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t sequence_ctrl;
    uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0];
} wifi_ieee80211_packet_t;

int sniff(int duration, uint16_t *type_filter, int verbose, uint8_t *channels, int channel_count);

void filter_set_multi(
    uint8_t src_list[][6], int src_count,
    uint8_t dst_list[][6], int dst_count,
    uint8_t ap_list[][6],  int ap_count
);

typedef struct {
    uint16_t include[16];
    int include_count;
    uint16_t exclude[16];
    int exclude_count;
} fc_filter_t;

extern fc_filter_t active_fc_filter;

bool fc_passes_filter(uint16_t fc, const fc_filter_t *filter);
fc_filter_t parse_fc_filters(const char *csv);

#ifdef __cplusplus
}
#endif

#endif