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
                clearScreen();
                vTaskDelay(pdMS_TO_TICKS(300));
                int wait = 1;
                switch (Selector) {
                    case 0:
                        displayText(0, 0, "Current Firmware:");
                        displayText(0, 20, VERSION);
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            updateBoard();
                            if (anyPressed()) {
                                wait = 0;
                            }

                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        clearScreen();
                        break;
                    case 1:
                        displayText(0, 0, "Nothing");
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            updateBoard();
                            if (anyPressed()) {
                                wait = 0;
                            }
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        clearScreen();
                        break;
                    case 2:
                        displayText(0, 40, "Good job,");
                        displayText(0, 60, "you develop");
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            updateBoard();
                            if (anyPressed()) {
                                wait = 0;
                            }
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        clearScreen();
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}