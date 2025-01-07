#include "app_wifi_sniffer.h"

int App_Wifi_Sniffer() {
    start_wifi(WIFI_MODE_STA, true);

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFi/WifiSniffer";
    Menu.length = 2;  // filter, statasniffing
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "Filter");
    Menu.elements[0].type = 1;
    Menu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[0].options[i] = NULL;
    }

    strcpy(Menu.elements[1].name, "Start sniffing");
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    uint16_t *filters    = NULL;
    int filter_count = 0;

    while (1) {
        updateBoard();
        if (anyPressed()) {
            if (returnPressed()) {
                stop_wifi();
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
                clearScreen();

                switch (Selector) {
                    case 0: //filter
                        filters = select_filter_menu(&filter_count, t_filter, t_filter_count);
                        selected_t_filter_count = filter_count;
                        break;
                    case 1: // Start sniffing
                        vTaskDelay(pdMS_TO_TICKS(100));
                        init_sniff_pps_timer();
                        sniff(1000, filters, 1);
                        stop_sniff_pps_timer();
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
