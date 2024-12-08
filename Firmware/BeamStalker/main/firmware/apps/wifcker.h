#ifndef WIFCKER_H
#define WIFCKER_H

#include <M5Cardputer.h>

#include "firmware/menu.h"

#define CHANNEL 11

int charset;

char *generate_ssid() {
    char *ssid;

    if (charset == 3) {
        const char *base_ssid = "CamionDeLaDGSI"; // Use const char* for literals
        int rand_num = rand() % 9998;

        char buffer[10];
        int num_digits = snprintf(buffer, sizeof(buffer), "%d", rand_num);

        size_t new_length = strlen(base_ssid) + 1 + num_digits + 1;
        char *new_ssid = (char *)malloc(new_length); // Explicit cast for malloc

        snprintf(new_ssid, new_length, "%s-%d", base_ssid, rand_num);
        return new_ssid;
    }

    const char *hiragana[] = {"あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と"};
    const char *katakana[] = {"ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", "サ", "シ", "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト"};
    const char *cyrillic[] = {"А", "Б", "В", "Г", "Д", "Е", "Ё", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я"};

    int num_chars = 8;
    ssid = (char *)malloc(num_chars * 3 + 1);

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
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d packets/sec", packet_count);

    M5.Display.clear();
    M5.Display.setCursor(0, 20);
    M5.Display.println("Attacking...");
    M5.Display.println(buffer);

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

int BeaconSpam() {
    srand(time(NULL));
    charset = 1;

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFcker/Beacon Spam";
    Menu.length = 2;  // charset, statack
    Menu.elements = new item[Menu.length];

    Menu.elements[0].name = "Charset";
    Menu.elements[0].type = 0;
    Menu.elements[0].length = 4;
    Menu.elements[0].options[0] = "hig";
    Menu.elements[0].options[1] = "kat";
    Menu.elements[0].options[2] = "cyr";
    Menu.elements[0].options[3] = "CDLD";
    Menu.elements[0].options[4] = NULL; // Terminate options explicitly


    Menu.elements[1].name = "Start attack";
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
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
                M5GFX_clear_screen();
                switch (Selector) {
                    case 1:  // Start attack
                        init_pps_timer();
                        vTaskDelay(pdMS_TO_TICKS(100));
                        int wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            packet_count++;
                            troll();
                            for (int i = 0; i < 10; i++) {
                                vTaskDelay(pdMS_TO_TICKS(0));  // Yield the processor for a short duration
                            }
                        }
                        stop_pps_timer();
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }


}

int menuTask() {
    srand(time(NULL));
    charset = 1;

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFcker";
    Menu.length = 2;  // BeaconSpam, Deauther
    Menu.elements = new item[Menu.length];

    Menu.elements[0].name = "Beacon Spam";
    Menu.elements[0].type = 1;
    Menu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[0].options[i] = NULL;
    }

    Menu.elements[1].name = "Deauther";
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    int UPp, DOWNp, SELECTp, RETURNp;

    while (1) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange()) {
            UPp = M5Cardputer.Keyboard.isKeyPressed(';');
            DOWNp = M5Cardputer.Keyboard.isKeyPressed('.');
//            LEFTp = M5Cardputer.Keyboard.isKeyPressed(',');
//            RIGHTp = M5Cardputer.Keyboard.isKeyPressed('/');
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
            if (SELECTp) {
                switch (Selector) {
                    int ret;
                    case 0:  // BeaconSpam
                        M5GFX_clear_screen();
                        printf ("beacon_spam_task - starting");
                        ret = BeaconSpam();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                    case 1:  // Deauther
                        M5GFX_clear_screen();
                        ret = 0;
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int APP_WiFcker() {
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

    ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(20));

    int ret = menuTask();

    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());

    return ret;
}
#endif
