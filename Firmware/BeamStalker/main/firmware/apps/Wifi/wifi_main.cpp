#include "wifi_main.h"

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

                        start_wifi(WIFI_MODE_STA, true);

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

                        stop_wifi();

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
    int ret = menuTask();

    return ret;
}
