#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "freertos/timers.h"

#include "../menu.h"
#include "wifi.h"

#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)
#define WIFI_CHANNEL_MAX               (13)
#define MAX_AP_COUNT                   50
#define MAX_CLIENT_COUNT               20
#define MAX_CLIENTS_PER_AP             20
#define BROADCAST_MAC { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }

uint8_t level = 0, channel = 11;

uint16_t t_filter[] = {
    0x0000, 0x0010, 0x0020, 0x0030, 0x0040, 0x0050, 0x0060, 0x0070, 0x0080, 0x0090, 0x00a0, 0x00b0, 0x00c0,
    0x00d0, 0x00e0, 0x00f0, 0x0100, 0x0110, 0x0120, 0x0130, 0x0140, 0x0150, 0x0160, 0x0170, 0x0180, 0x0190,
    0x01a0, 0x01b0, 0x01c0, 0x01d0, 0x01e0, 0x01f0, 0x0200, 0x0210, 0x0220, 0x0230, 0x0240, 0x0250, 0x0260,
    0x0270, 0x0280, 0x0290, 0x02a0, 0x02b0, 0x02c0, 0x02d0, 0x02e0, 0x02f0, 0x0300, 0x0310, 0x0320, 0x0330,
    0x0340, 0x0350, 0x0360, 0x0370, 0x0380, 0x0390, 0x03a0, 0x03b0, 0x03c0, 0x03d0, 0x03e0, 0x03f0    
}; 

size_t t_filter_count = (sizeof(t_filter) / sizeof(t_filter[0]));
uint16_t *selected_t_filter = NULL;
int selected_t_filter_count;

int sniffer_verbose = 0;
int sniff_packet_count;

typedef struct {
    uint8_t mac[6];
} mac_addr_t;

typedef struct {
    mac_addr_t ap_mac;
    mac_addr_t clients[MAX_CLIENTS_PER_AP];
    uint8_t client_count;
} ap_info_t;

ap_info_t sniff_ap_list[MAX_AP_COUNT];
uint8_t sniff_ap_count = 0;

typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    unsigned sequence_ctrl:16;
    uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0];
} wifi_ieee80211_packet_t;

void sniff_pps_timer_callback(TimerHandle_t xTimer);
void init_sniff_pps_timer();
void stop_sniff_pps_timer();
bool mac_equals(const uint8_t *mac1, const uint8_t *mac2);
bool is_broadcast(const uint8_t *mac);
void add_client_to_ap(const uint8_t *ap_mac, const uint8_t *client_mac);
void add_ap_if_new(const uint8_t *ap_mac);
void sniffer_log(const wifi_ieee80211_mac_hdr_t *hdr);
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
void wifi_sniffer_init(void);
void wifi_sniffer_set_channel(uint8_t channel);
int sniff(int duration, uint16_t *type_filter, int verbose);
mac_addr_t* getSelectedClients(menu Menu, ap_info_t* ap_info, int* selected_count);
mac_addr_t* select_client_menu(int *selected_ap_count, AP* aps, int aps_count);
uint16_t* getSelectedFilter(menu Menu, uint16_t* filter, int* selected_count);
uint16_t* select_filter_menu(int *selected_filter_count, uint16_t *filters, int filter_count);

#endif
