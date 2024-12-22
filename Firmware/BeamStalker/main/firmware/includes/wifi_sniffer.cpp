#include "wifi_sniffer.h"

bool mac_equals(const uint8_t *mac1, const uint8_t *mac2) {
    for (int i = 0; i < 6; i++) {
        if (mac1[i] != mac2[i]) {
            return false;
        }
    }
    return true;
}

bool is_broadcast(const uint8_t *mac) {
    const uint8_t broadcast_mac[6] = BROADCAST_MAC;
    return mac_equals(mac, broadcast_mac);
}

void add_client_to_ap(const uint8_t *ap_mac, const uint8_t *client_mac) {
    for (int i = 0; i < sniff_ap_count; i++) {
        if (mac_equals(sniff_ap_list[i].ap_mac.mac, client_mac)) {
            return;
        }
    }
    for (int i = 0; i < sniff_ap_count; i++) {
        if (mac_equals(sniff_ap_list[i].ap_mac.mac, ap_mac)) {
            for (int j = 0; j < sniff_ap_list[i].client_count; j++) {
                if (mac_equals(sniff_ap_list[i].clients[j].mac, client_mac)) {
                    return; // Client already exists
                }
            }

            // Add new client
            if (sniff_ap_list[i].client_count < MAX_CLIENT_COUNT) {
                memcpy(sniff_ap_list[i].clients[sniff_ap_list[i].client_count].mac, client_mac, 6);
                sniff_ap_list[i].client_count++;
                printf ("HIT for AP %d [%02x:%02x], got client %d\n", i, sniff_ap_list[i].ap_mac.mac[4], sniff_ap_list[i].ap_mac.mac[5],sniff_ap_list[i].client_count);
                fflush(stdout);
            }
            return;
        }
    }
}

void add_ap_if_new(const uint8_t *ap_mac) {
    for (int i = 0; i < sniff_ap_count; i++) {
        if (mac_equals(sniff_ap_list[i].ap_mac.mac, ap_mac)) {
            return; // AP already known
        }
    }

    if (sniff_ap_count < MAX_AP_COUNT) {
        memcpy(sniff_ap_list[sniff_ap_count].ap_mac.mac, ap_mac, 6);
        sniff_ap_list[sniff_ap_count].client_count = 0;
        sniff_ap_count++;
    }
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    uint16_t fc = hdr->frame_ctrl;

//    uint8_t frame_category = (fc & 0x000C) >> 2;
    uint8_t frame_action = (fc & 0x00F0) >> 4;

    // Check for Beacon Frames (AP identification)
    if (frame_action == 0x08) {
        if (is_broadcast(hdr->addr1)) {
            add_ap_if_new(hdr->addr2); // addr2 is the BSSID (AP MAC)
        }
    }

    if (is_broadcast(hdr->addr1) || is_broadcast(hdr->addr2) || is_broadcast(hdr->addr3)) {
    } else {
        // For all frames, check if an AP is involved
        for (int i = 0; i < sniff_ap_count; i++) {
            if (mac_equals(hdr->addr1, sniff_ap_list[i].ap_mac.mac)) {
                if (!mac_equals(hdr->addr2, sniff_ap_list[i].ap_mac.mac) && mac_equals(hdr->addr1, hdr->addr3)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr2);
                }
            }
            if (mac_equals(hdr->addr2, sniff_ap_list[i].ap_mac.mac)) {
                if (!mac_equals(hdr->addr1, sniff_ap_list[i].ap_mac.mac) && (memcmp(hdr->addr2, hdr->addr3, 3) == 0)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr1);
                }
            }
            if (mac_equals(hdr->addr3, sniff_ap_list[i].ap_mac.mac)) {
                if (!mac_equals(hdr->addr2, sniff_ap_list[i].ap_mac.mac) && mac_equals(hdr->addr1, hdr->addr3)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr2);
                }
                if (!mac_equals(hdr->addr1, sniff_ap_list[i].ap_mac.mac) && mac_equals(hdr->addr2, hdr->addr3)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr1);
                }
            }
        }
    }
}

void wifi_sniffer_init(void) {
    //tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel) {
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

int sniff(int duration) {
    stop_wifi();
    vTaskDelay(pdMS_TO_TICKS(100));
    wifi_sniffer_init();
    time_t start_time = time(NULL);
    time_t end_time = start_time + duration;

    while (time(NULL) < end_time) {
        wifi_sniffer_set_channel(channel);
        channel = (channel % WIFI_CHANNEL_MAX) + 1;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    stop_wifi();
    start_wifi(WIFI_MODE_STA, true);

    return 0;
}

mac_addr_t* getSelectedClients(menu Menu, ap_info_t* ap_info, int* selected_count) {
    mac_addr_t* selected_clients = (mac_addr_t*)malloc(Menu.length * sizeof(mac_addr_t));
    if (!selected_clients) {
        printf("Memory allocation failed for selected Clients.\n");
        return NULL;
    }

    int count = 0;

    for (int i = 0; i < Menu.length - 1; i++) { // -2 for ff:ff...ff:ff and Select
        if (Menu.elements[i].options[Menu.elements[i].selector][0] == 'x') {
            memcpy(&selected_clients[count], &ap_info->clients[i], sizeof(mac_addr_t));
            if (i == Menu.length - 2) {
                for (int j = 0; j < 6; j++) {
                    selected_clients[count].mac[j] = (uint8_t)0xff; // Broadcast MAC
                }   
            }
            count++;
        }
    }

    selected_clients = (mac_addr_t*)realloc(selected_clients, count * sizeof(mac_addr_t));

    *selected_count = count;
    return selected_clients;
}

mac_addr_t* select_client_menu(int *selected_client_count, AP* aps, int aps_count) {
    sniff(10);
    int length = 0;
    for (int i = 0; i < sniff_ap_count; i++) {
        if (mac_equals(sniff_ap_list[i].ap_mac.mac, aps->address)) {
            for (int j = 0; j < sniff_ap_list[i].client_count; j++) {
                length++;
            }
        }
    }

    // LogError("AP found: "  + std::to_string(sniff_ap_count)
    //         + "\nvanilla AP found: " + std::to_string(aps_count)
    //         + "\nLength"  + std::to_string(length)
    // );
    
    int itemCount = 0;
    int Selector = 0;
    menu Menu;
    Menu.name = "Client Select";
    Menu.length = length + 1 + 1; // boradcast, Select
    Menu.elements = (item *)malloc(Menu.length * sizeof(item));

    for (int i = 0; i < sniff_ap_count; i++) {
        for (int k = 0; k < aps_count; k++) {
            if (mac_equals(sniff_ap_list[i].ap_mac.mac, aps[k].address)) {
                for (int j = 0; j < sniff_ap_list[i].client_count; j++) {
                    snprintf(Menu.elements[itemCount].name, sizeof(Menu.elements[itemCount].name),
                    
                                "%02x:%02x...%02x:%02x",
                                
                                sniff_ap_list[i].clients[j].mac[0], sniff_ap_list[i].clients[j].mac[1],
    //                          sniff_ap_list[i].clients[j].mac[2], sniff_ap_list[i].clients[j].mac[3],
                                sniff_ap_list[i].clients[j].mac[4], sniff_ap_list[i].clients[j].mac[5]
                                );
                    Menu.elements[itemCount].type = 0;
                    Menu.elements[itemCount].length = 2;
                    Menu.elements[itemCount].selector = 0;
                    Menu.elements[itemCount].options[0] = " ";
                    Menu.elements[itemCount].options[1] = "x";
                    Menu.elements[itemCount].options[2] = NULL;

                    itemCount++;
                }
            }
        }
    }

    strcpy(Menu.elements[Menu.length-2].name, "ff:ff...ff:ff");
    Menu.elements[Menu.length-2].type = 0;
    Menu.elements[Menu.length-2].length = 2;
    Menu.elements[Menu.length-2].selector = 0;
    Menu.elements[Menu.length-2].options[0] = " ";
    Menu.elements[Menu.length-2].options[1] = "x";
    Menu.elements[Menu.length-2].options[2] = NULL;

    strcpy(Menu.elements[Menu.length-1].name, "Select");
    Menu.elements[Menu.length-1].type = 1;
    Menu.elements[Menu.length-1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[Menu.length-1].options[i] = NULL;
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
                    mac_addr_t* selected_clients = getSelectedClients(Menu, sniff_ap_list, selected_client_count);

                    return selected_clients;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}