#include "ble_main.h"

int bleMenuTask() {
    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/BLE";
    Menu.length = 1;  // BLESpam
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "BLE Spam");
    Menu.elements[0].type = 1;
    Menu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[0].options[i] = NULL;
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
            if (SELECTp) {
                vTaskDelay(pdMS_TO_TICKS(300));

                switch (Selector) {
                    int ret;
                    case 0:  // BLESpam
                        printf ("ble_spam_task - starting\n");
                        ret = BLESpam();
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

int APP_BLE() {
    int ret = bleMenuTask();

    return ret;
}
