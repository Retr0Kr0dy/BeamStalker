#include "app_wifi_sniffer.h"

int App_Wifi_Sniffer() {
    start_wifi(WIFI_MODE_STA, true);

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFi/Wifi sniffer";
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

    int UPp, DOWNp, SELECTp, RETURNp;
    uint16_t *filters    = NULL;
    int filter_count = 0;

    while (1) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isPressed()) {
            UPp = M5Cardputer.Keyboard.isKeyPressed(';');
            DOWNp = M5Cardputer.Keyboard.isKeyPressed('.');
            // LEFTp = M5Cardputer.Keyboard.isKeyPressed(',');
            // RIGHTp = M5Cardputer.Keyboard.isKeyPressed('/');
            SELECTp = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
            RETURNp = M5Cardputer.Keyboard.isKeyPressed('`');

            if (RETURNp) {
                stop_wifi();

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
            //else if (LEFTp && (Menu.elements[Selector].type == 0)) {
            //     Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector - 1, Menu.elements[Selector].length);
            //     vTaskDelay(pdMS_TO_TICKS(50));
            // }
            // else if (RIGHTp  && (Menu.elements[Selector].type == 0)) {
            //     Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector + 1, Menu.elements[Selector].length);
            //     vTaskDelay(pdMS_TO_TICKS(50));
            // }
            if (SELECTp) {
                M5GFX_clear_screen();
                switch (Selector) {
                    case 0: //filter
                        filters = select_filter_menu(&filter_count, t_filter, t_filter_count);
                        selected_t_filter_count = filter_count;
                        break;
                    case 1: // Start sniffing
                        vTaskDelay(pdMS_TO_TICKS(100));
                        
                        M5GFX_display_text(0, 0, "Sniffing for 60s !\nPress any key to exit...", TFT_WHITE);
                        sniff(60, filters, 1);
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
