#ifndef WIFI_SNIFFER_H
#define WIFI_SNIFFER_H

#include <M5Cardputer.h>

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

bool mac_equals(const uint8_t *mac1, const uint8_t *mac2);
bool is_broadcast(const uint8_t *mac);
void add_client_to_ap(const uint8_t *ap_mac, const uint8_t *client_mac);
void add_ap_if_new(const uint8_t *ap_mac);
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type);
void wifi_sniffer_init(void);
void wifi_sniffer_set_channel(uint8_t channel);
int sniff(int duration);
mac_addr_t* getSelectedClients(menu Menu, ap_info_t* ap_info, int* selected_count);
mac_addr_t* select_client_menu(int *selected_ap_count, AP* aps, int aps_count);

#endif
