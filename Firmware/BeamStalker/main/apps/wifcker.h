#ifndef WIFCKER_H
#define WIFCKER_H

#define CHANNEL 11
//#define MAX_SSID_LEN 32
//#define LEN 16

int charset;

char *generate_ssid() {
    char *ssid;
    if (charset == 3) {
        ssid = "​​CamionDeLaDGSI";
        int rand_num = rand() % 9998;

        char buffer[10];
        int num_digits = snprintf(buffer, sizeof(buffer), "%d", rand_num);

        size_t new_length = strlen(ssid) + 1 + num_digits + 1;
        char *new_ssid = (char *)malloc(new_length);

        snprintf(new_ssid, new_length, "%s-%d", ssid, rand_num);
        return new_ssid;
    }

    const char *hiragana[] = {
        "あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", "さ", "し",
        "す", "せ", "そ", "た", "ち", "つ", "て", "と"
};
    const char *katakana[] = {
        "ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", "サ", "シ",
        "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト"
};
    const char *cyrillic[] = {
        "А", "Б", "В", "Г", "Д", "Е", "Ё", "Ж", "З", "И", "Й", "К", "Л", "М",
        "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ",
        "Ы", "Ь", "Э", "Ю", "Я"
};

    int num_chars = 8;
    ssid = malloc(num_chars * 3 + 1); // Up to 3 bytes per character + null terminator

    if (!ssid) return NULL;

    ssid[0] = '\0';

    for (int i = 0; i < num_chars; i++) {
        const char *char_utf8;
        switch (charset) {
            case 0:
                char_utf8 = hiragana[rand() % (sizeof(hiragana) / sizeof(hiragana[0]))];
                break;
            case 1:
                char_utf8 = katakana[rand() % (sizeof(katakana) / sizeof(katakana[0]))];
                break;
            case 2:
            default:
                char_utf8 = cyrillic[rand() % (sizeof(cyrillic) / sizeof(cyrillic[0]))];
                break;
        }
        strcat(ssid, char_utf8);
    }

    return ssid;
}

void generate_random_mac(uint8_t *mac_addr) {
    mac_addr[0] = 0x02; // Set as locally administered (bit 1 of MSB = 1)
    for (int i = 1; i < 6; i++) {
        mac_addr[i] = rand() % 256;
    }
}

void send_beacon(const char *ssid, const uint8_t *mac_addr, uint8_t channel) {
    uint8_t beacon_frame[128] = {0};
    int ssid_len = strlen(ssid);
    int frame_len = 0;

    // Frame Control (Type: Management, Subtype: Beacon)
    beacon_frame[0] = 0x80;
    beacon_frame[1] = 0x00;

    // Duration
    beacon_frame[2] = 0xff;
    beacon_frame[3] = 0xff;

    // Destination MAC (broadcast)
    memset(beacon_frame + 4, 0xff, 6);

    // Source MAC (provided MAC address)
    memcpy(beacon_frame + 10, mac_addr, 6);

    // BSSID (same as source MAC for this beacon)
    memcpy(beacon_frame + 16, mac_addr, 6);

    // Sequence Control
    beacon_frame[22] = 0xc0;
    beacon_frame[23] = 0x6c;

    // Timestamp (8 bytes, set to zero for simplicity)
    memset(beacon_frame + 24, 0x01, 8);

    // Beacon Interval
    beacon_frame[32] = 0x64; // 0x0064 = 100 TU
    beacon_frame[33] = 0x00;

    // Capabilities Information
    beacon_frame[34] = 0x01;
    beacon_frame[35] = 0x04;

    frame_len = 36;

    // SSID Parameter Set (Tag Number: 0, Tag Length: ssid_len)
    beacon_frame[frame_len++] = 0x00;     // Tag Number: SSID parameter set
    beacon_frame[frame_len++] = ssid_len; // Tag Length
    memcpy(beacon_frame + frame_len, ssid, ssid_len);
    frame_len += ssid_len;

    // Supported Rates (Tag Number: 1, Tag Length: 8)
    beacon_frame[frame_len++] = 0x01;     // Tag Number: Supported Rates
    beacon_frame[frame_len++] = 0x08;     // Tag Length
    uint8_t supported_rates[] = {0x82, 0x84, 0x8b, 0x96, 0x12, 0x24, 0x48, 0x6c};
    memcpy(beacon_frame + frame_len, supported_rates, sizeof(supported_rates));
    frame_len += sizeof(supported_rates);

    // DS Parameter Set (Tag Number: 3, Tag Length: 1)
    beacon_frame[frame_len++] = 0x03;     // Tag Number: DS Parameter Set
    beacon_frame[frame_len++] = 0x01;     // Tag Length
    beacon_frame[frame_len++] = channel;  // Channel number

    esp_wifi_80211_tx(WIFI_IF_STA, beacon_frame, frame_len, false);
}

void troll() {
    char *ssid = generate_ssid();

    if (ssid == NULL) {
        printf("Failed to generate SSID\n");
        return;
    }

    uint8_t mac_addr[6];
    generate_random_mac(mac_addr);

    for (int p = 0; p < 1; p++) {
        send_beacon(ssid, mac_addr, CHANNEL);
    }

    free(ssid);
}

static int packet_count = 0;
TimerHandle_t pps_timer;

void pps_timer_callback(TimerHandle_t xTimer) {
    char *counter_text = " packets/sec";
    size_t buf_size = snprintf(NULL, 0, "%d%s", packet_count, counter_text) + 1;
    char *buf = malloc(buf_size);

    snprintf(buf, buf_size, "%d%s", packet_count, counter_text);

    ssd1306_clear_screen(&dev, false);
    ssd1306_display_text(&dev, 2, "Attacking...", 12, false);
    ssd1306_display_text(&dev, 4, buf, strlen(buf), false);

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
        if (xTimerStop(pps_timer, 0) != pdPASS) {
            printf("Failed to stop the PPS timer\n");
        }
        if (xTimerDelete(pps_timer, 0) != pdPASS) {
            printf("Failed to delete the PPS timer\n");
        }
        pps_timer = NULL;
    } else {
        printf("PPS timer is not running or was never created\n");
    }
}

int sendBeaconTask() {
//    esp_task_wdt_delete(NULL);  // This deletes the watchdog for the current task

    int state = 0;

    srand(time(NULL));

    charset = 1;

    char *charset_text = "Charset: ";

    int UPp, DOWNp, SELECTp, RETURNp;

    for (;;) {
        size_t buf_size = snprintf(NULL, 0, "%s%d     ", charset_text, charset) + 1;
        char *charset_str = malloc(buf_size);

        snprintf(charset_str, buf_size, "%s%d     ", charset_text, charset);

        ssd1306_display_text(&dev, 0, charset_str, buf_size, false);
        ssd1306_display_text(&dev, 2, "Press select      ", 18, false);
        ssd1306_display_text(&dev, 4, "to start...     ", 16, false);

        UPp = !gpio_get_level(UP_GPIO);
        DOWNp = !gpio_get_level(DOWN_GPIO);
        SELECTp = !gpio_get_level(SELECT_GPIO);
        RETURNp = !gpio_get_level(RETURN_GPIO);

        if (UPp) {
            charset = intChecker(charset+1,4);
            vTaskDelay(pdMS_TO_TICKS(100));
            ssd1306_clear_screen(&dev, false);
        }
        if (DOWNp) {
            charset = intChecker(charset-1,4);
            vTaskDelay(pdMS_TO_TICKS(100));
            ssd1306_clear_screen(&dev, false);
        }
        if (SELECTp) {
            init_pps_timer();

            ssd1306_clear_screen(&dev, false);
            ssd1306_display_text(&dev, 4, "Starting...     ", 16, false);

            vTaskDelay(pdMS_TO_TICKS(300));

            while ((gpio_get_level(UP_GPIO)&&gpio_get_level(DOWN_GPIO)&&gpio_get_level(SELECT_GPIO)&&gpio_get_level(RETURN_GPIO))) {
                packet_count++;
                state = !state;
                gpio_set_level(LED_GPIO, state);

                troll();
                for (int i = 0; i < 10; i++) {
                    vTaskDelay(pdMS_TO_TICKS(0));  // Yield the processor for a short duration
                }
            }

            state = 0;
            gpio_set_level(LED_GPIO, state);

            stop_pps_timer();
            vTaskDelay(pdMS_TO_TICKS(100));

            ssd1306_clear_screen(&dev, false);
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        free(charset_str);

        if (RETURNp) {
            return 0;
        }
    }
}

int APP_WiFcker() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    int ret = sendBeaconTask();

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    return ret;
}
#endif
