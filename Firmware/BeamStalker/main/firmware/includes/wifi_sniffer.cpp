#include "wifi_sniffer.h"

void sniff_pps_timer_callback(TimerHandle_t xTimer) {
    char pc_buffer[32];
    snprintf(pc_buffer, sizeof(pc_buffer), "Packet: %d", sniff_packet_count);
    char ch_buffer[32];
    snprintf(ch_buffer, sizeof(ch_buffer), "Channel: %d", channel);

    M5.Display.clear();
    displayText(0, 0*charsize, "Sniffing for 1000s", TFT_WHITE);
    displayText(0, 2*charsize, pc_buffer, TFT_WHITE);
    displayText(0, 3*charsize, ch_buffer, TFT_WHITE);
    displayText(0, 7*charsize, "Press any key to exit...", TFT_WHITE);
}

void init_sniff_pps_timer() {
    sniff_packet_count = 0;
    pps_timer = xTimerCreate("PPS_Timer", pdMS_TO_TICKS(1000), pdTRUE, (void *)0, sniff_pps_timer_callback);
    if (pps_timer == NULL) {
        printf("Failed to create timer\n");
    } else {
        xTimerStart(pps_timer, 0);
    }
}

void stop_sniff_pps_timer() {
    if (pps_timer != NULL) {
        xTimerStop(pps_timer, 0);
        xTimerDelete(pps_timer, 0);
        pps_timer = NULL;
    }
}

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

void sniffer_log(const wifi_ieee80211_mac_hdr_t *hdr) {
    sniff_packet_count++;

    printf("frame_ctrl: %04x, duration_id: %u, "
        "addr1: %02x:%02x:%02x:%02x:%02x:%02x, "
        "addr2: %02x:%02x:%02x:%02x:%02x:%02x, "
        "addr3: %02x:%02x:%02x:%02x:%02x:%02x, "
        "sequence_ctrl: %u, "
        "addr4: %02x:%02x:%02x:%02x:%02x:%02x\n",
        hdr->frame_ctrl,
        hdr->duration_id,
        hdr->addr1[0], hdr->addr1[1], hdr->addr1[2], hdr->addr1[3], hdr->addr1[4], hdr->addr1[5],
        hdr->addr2[0], hdr->addr2[1], hdr->addr2[2], hdr->addr2[3], hdr->addr2[4], hdr->addr2[5],
        hdr->addr3[0], hdr->addr3[1], hdr->addr3[2], hdr->addr3[3], hdr->addr3[4], hdr->addr3[5],
        hdr->sequence_ctrl,
        hdr->addr4[0], hdr->addr4[1], hdr->addr4[2], hdr->addr4[3], hdr->addr4[4], hdr->addr4[5]);

}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    uint16_t fc = hdr->frame_ctrl;

    uint8_t frame_category = (fc & 0x000C) >> 2;
    uint8_t frame_action = (fc & 0x00F0) >> 4;

    if (sniffer_verbose) {
        if (selected_t_filter == NULL) {
            sniffer_log(hdr);
        } else {
            for (int i = 0; i < selected_t_filter_count; i ++) {
                if (fc == selected_t_filter[i]) {
                    sniffer_log(hdr);
                }
            }
        }
    }
    
    // Check for Beacon Frames (AP identification)
    if (frame_category == 0x00 && frame_action == 0x08) {
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

int sniff(int duration, uint16_t *type_filter, int verbose) {
    selected_t_filter = type_filter;
    sniffer_verbose = verbose;

    stop_wifi();
    vTaskDelay(pdMS_TO_TICKS(500));
    wifi_sniffer_init();
    time_t start_time = time(NULL);
    time_t end_time = start_time + duration;

    while (time(NULL) < end_time) {
        if (verbose == 1) { // if using wifi sniffer app, need to catch key press for exit
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isPressed()) {
                break;
            }
        }    
        wifi_sniffer_set_channel(channel);
        channel = (channel % WIFI_CHANNEL_MAX) + 1;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    stop_wifi();
    vTaskDelay(pdMS_TO_TICKS(100));
    start_wifi(WIFI_MODE_STA, true);

    if (verbose) {
        sniffer_verbose = 0;
    }

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
    sniff(10, NULL, 0);
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

    while (1) {
        updateBoard();
        if (anyPressed()) {
            if (returnPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
                return 0;
            }
           else if (upPressed()) {
                Selector = intChecker(Selector - 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (downPressed()) {
                Selector = intChecker(Selector + 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (leftPressed() && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector - 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (rightPressed()  && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector + 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            if (selectPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
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

uint16_t* getSelectedFilter(menu Menu, uint16_t* filter, int* selected_count) {
    uint16_t* selected_filter = (uint16_t*)malloc(Menu.length * sizeof(uint16_t));
    if (!selected_filter) {
        printf("Memory allocation failed for selected Filter.\n");
        return NULL;
    }

    int count = 0;


    if (Menu.elements[0].options[Menu.elements[0].selector][0] == 'x') {
        for (int i = 0; i < t_filter_count; i++) {
            selected_filter[i] = t_filter[i];
        }
    } else {
        for (int i = 0; i < Menu.length - 1; i++) {
            if (Menu.elements[i].options[Menu.elements[i].selector][0] == 'x') {
                memcpy(&selected_filter[count], &filter[i-1], sizeof(uint16_t));
                count++;
            }
        }
    }

    selected_filter = (uint16_t*)realloc(selected_filter, count * sizeof(uint16_t));

    *selected_count = count;
    return selected_filter;
}

uint16_t* select_filter_menu(int *selected_filter_count, uint16_t *filters, int filter_count) {
    int itemCount = 0;
    int Selector = 0;
    menu Menu;
    Menu.name = "Filter Select";
    Menu.length = t_filter_count + 1 + 1; // all, Select
    Menu.elements = (item *)malloc(Menu.length * sizeof(item));

    strcpy(Menu.elements[0].name, "all");
    Menu.elements[0].type = 0;
    Menu.elements[0].length = 2;
    Menu.elements[0].selector = 0;
    Menu.elements[0].options[0] = " ";
    Menu.elements[0].options[1] = "x";
    Menu.elements[0].options[2] = NULL;

    itemCount++;

    for (int i = 0; i < filter_count; i++) {
        snprintf(Menu.elements[itemCount].name, sizeof(Menu.elements[itemCount].name), "%04x", filters[i]);
        Menu.elements[itemCount].type = 0;
        Menu.elements[itemCount].length = 2;
        Menu.elements[itemCount].selector = 0;
        Menu.elements[itemCount].options[0] = " ";
        Menu.elements[itemCount].options[1] = "x";
        Menu.elements[itemCount].options[2] = NULL;

        itemCount++;
    }

    strcpy(Menu.elements[Menu.length-1].name, "Select");
    Menu.elements[Menu.length-1].type = 1;
    Menu.elements[Menu.length-1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[Menu.length-1].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    while (1) {
        updateBoard();
        if (anyPressed()) {
            if (returnPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
                return 0;
            }
           else if (upPressed()) {
                Selector = intChecker(Selector - 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (downPressed()) {
                Selector = intChecker(Selector + 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (leftPressed() && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector - 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (rightPressed()  && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector + 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            if (selectPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
                if (Selector == (Menu.length - 1)) {  // Select
                    uint16_t* selected_filters = getSelectedFilter(Menu, filters, selected_filter_count);

                    selected_t_filter_count = *selected_filter_count;

                    return selected_filters;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}