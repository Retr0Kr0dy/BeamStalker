#include "options.h"

int APP_Options() {
    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/Options";
    Menu.length = 3;  // sysinfo, settings, develop
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "System Info");
    Menu.elements[0].type = 1;
    Menu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[0].options[i] = NULL;
    }

    strcpy(Menu.elements[1].name, "Settings");
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
    }

    strcpy(Menu.elements[2].name, "Developper");
    Menu.elements[2].type = 1;
    Menu.elements[2].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[2].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    int UPp, DOWNp, SELECTp, RETURNp;

    while (1) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isPressed()) {
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
                M5GFX_clear_screen();
                int wait = 1;
                switch (Selector) {
                    case 0:
                        M5GFX_display_text(0, 0, "Current Firmware:");
                        M5GFX_display_text(0, 20, VERSION);
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }

                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        M5GFX_clear_screen();
                        break;
                    case 1:
                        M5GFX_display_text(0, 0, "Nothing");
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        M5GFX_clear_screen();
                        break;
                    case 2:
                        M5GFX_display_text(0, 40, "Good job,");
                        M5GFX_display_text(0, 60, "you develop");
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        M5GFX_clear_screen();
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}