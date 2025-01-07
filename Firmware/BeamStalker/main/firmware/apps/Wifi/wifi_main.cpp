#include "wifi_main.h"

int wifiMenuTask() {
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

    strcpy(Menu.elements[2].name, "Wifi Sniffer");
    Menu.elements[2].type = 1;
    Menu.elements[2].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[2].options[i] = NULL;
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
            if (selectPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
                switch (Selector) {
                    int ret;
                    case 0:  // BeaconSpam
                        printf ("beacon_spam_task - starting\n");
                        ret = BeaconSpam();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                    case 1:  // Deauther
                        printf ("deauther_task - starting\n");
                        ret = Deauther();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                    case 2: // Sniff Wifi
                        printf ("wifi_sniffer_task - starting\n");
                        ret = App_Wifi_Sniffer();
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
    int ret = wifiMenuTask();

    return ret;
}
