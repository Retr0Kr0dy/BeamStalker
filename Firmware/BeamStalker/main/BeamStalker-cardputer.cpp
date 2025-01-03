extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "time.h"

#include "firmware/helper.h"
#include "firmware/bitmaps.h"
#include "firmware/menu.h"

#include "firmware/apps/options.h"
#include "firmware/apps/Wifi/wifi_main.h"
#include "firmware/apps/BLE/ble_main.h"

#include "M5GFX.h"
#include "M5Cardputer.h"

M5GFX display;
M5Canvas canvas(&display);

int mainTask() {
    vTaskDelay(pdMS_TO_TICKS(100));

    int MainMenuSelector = 0;

    struct menu MainMenu;

    MainMenu.name = "~/";
    MainMenu.length = 3;  // WiF, BLE, Opt
    MainMenu.elements = new item[MainMenu.length];

    strcpy(MainMenu.elements[0].name, "WiFi");
    MainMenu.elements[0].type = 1;
    MainMenu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        MainMenu.elements[0].options[i] = NULL;
    }

    strcpy(MainMenu.elements[1].name, "BLE");
    MainMenu.elements[1].type = 1;
    MainMenu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        MainMenu.elements[1].options[i] = NULL;
    }

    strcpy(MainMenu.elements[2].name, "Options");
    MainMenu.elements[2].type = 1;
    MainMenu.elements[2].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        MainMenu.elements[2].options[i] = NULL;
    }

    drawMenu(MainMenu, MainMenuSelector);

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
                MainMenuSelector = intChecker(MainMenuSelector - 1, MainMenu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (DOWNp) {
                MainMenuSelector = intChecker(MainMenuSelector + 1, MainMenu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (LEFTp && (MainMenu.elements[MainMenuSelector].type == 0)) {
                MainMenu.elements[MainMenuSelector].selector = intChecker(MainMenu.elements[MainMenuSelector].selector - 1, MainMenu.elements[MainMenuSelector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (RIGHTp  && (MainMenu.elements[MainMenuSelector].type == 0)) {
                MainMenu.elements[MainMenuSelector].selector = intChecker(MainMenu.elements[MainMenuSelector].selector + 1, MainMenu.elements[MainMenuSelector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }


            else if (SELECTp) {
                vTaskDelay(pdMS_TO_TICKS(50));

                switch (MainMenuSelector) {
                    int ret;
                    case 0:  // WiFcker
                        M5GFX_clear_screen();
                        ret = APP_WiFcker();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                    case 1:  // BLE
                        M5GFX_clear_screen();
                        ret = APP_BLE();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                    case 2:  // Options
                        M5GFX_clear_screen();
                        ret = APP_Options();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                }
            }
            drawMenu(MainMenu, MainMenuSelector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

extern "C" void app_main(void) {
    M5Cardputer.begin(true);
    M5.Power.begin();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    M5.Display.clear();
    M5.Display.setTextSize(charsize_multiplier);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::FreeMonoBold18pt7b);
    const char* name = "BeamStalker";

    for (;;) {
        M5.Display.clear();
        // int16_t x = (M5.Display.width() - 128) / 2;
        // drawBitmap(x, 0, 128, 64, skullLogo, TFT_WHITE);
        int16_t x = ((M5.Display.width() - 120) / 4) * 3;
        drawBitmap(x, 0, 120, 120, skully, TFT_WHITE);
        
        vTaskDelay(pdMS_TO_TICKS(200));

        M5.Display.setCursor(0, 70);
        M5.Display.print(name);
        M5.Display.setCursor(0, 90);
        M5.Display.print(VERSION);
        printf("%s %s\n", name, VERSION);

        vTaskDelay(pdMS_TO_TICKS(500));
        M5.Display.setCursor(0, 110);
        M5.Display.print("Press to boot...");

        int loop = 1;

        while (loop) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange()) {
                int wait = 1;
                while (wait) {
                    M5Cardputer.update();
                    if (M5Cardputer.Keyboard.isPressed()) {
                        wait = 0;
                    }
                    vTaskDelay(pdMS_TO_TICKS(30));
                }

                M5.Display.clear();

                int taskRet = mainTask();
                if (taskRet != 0) {
                    printf("Main task error\n");
                }
                loop = 0;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
