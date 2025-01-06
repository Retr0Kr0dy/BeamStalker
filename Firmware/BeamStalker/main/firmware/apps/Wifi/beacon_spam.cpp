#include "beacon_spam.h"

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

char *generate_ssid() {
    char *ssid;

    if (charset == 3) {
        const char *base_ssid = "CamionDeLaDGSI";
        int rand_num = rand() % 9998;

        char buffer[10];
        int num_digits = snprintf(buffer, sizeof(buffer), "%d", rand_num);

        size_t new_length = strlen(base_ssid) + 1 + num_digits + 1;
        char *new_ssid = (char *)malloc(new_length);

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

void trollBeacon() {
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

int BeaconSpam() {
    start_wifi(WIFI_MODE_STA, true);

    charset = 0;

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFi/BcnSpm";
    Menu.length = 2;  // charset, statack
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "Charset");
    Menu.elements[0].type = 0;
    Menu.elements[0].length = 4;
    Menu.elements[0].selector = 0;
    Menu.elements[0].options[0] = "hig";
    Menu.elements[0].options[1] = "kat";
    Menu.elements[0].options[2] = "cyr";
    Menu.elements[0].options[3] = "CDLD";
    Menu.elements[0].options[4] = NULL;


    strcpy(Menu.elements[1].name, "Start attack");
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    int UPp, DOWNp, LEFTp, RIGHTp, SELECTp, RETURNp;

    while (1) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isPressed()) {
            UPp = M5Cardputer.Keyboard.isKeyPressed(';');
            DOWNp = M5Cardputer.Keyboard.isKeyPressed('.');
            LEFTp = M5Cardputer.Keyboard.isKeyPressed(',');
            RIGHTp = M5Cardputer.Keyboard.isKeyPressed('/');
            SELECTp = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
            RETURNp = M5Cardputer.Keyboard.isKeyPressed('`');

            if (RETURNp) {
                stop_wifi();
                vTaskDelay(pdMS_TO_TICKS(300));

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
                vTaskDelay(pdMS_TO_TICKS(300));
                M5GFX_clear_screen();
                
                switch (Selector) {
                    case 1: // Start attack
                        init_pps_timer();
                        vTaskDelay(pdMS_TO_TICKS(100));
                        charset = Menu.elements[0].selector;
                        int wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            packet_count++;
                            trollBeacon();
                            for (int i = 0; i < 10; i++) {
                                vTaskDelay(pdMS_TO_TICKS(0));  // Yield the processor for a short duration
                            }
                        }
                        stop_pps_timer();
                        vTaskDelay(pdMS_TO_TICKS(300));
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
