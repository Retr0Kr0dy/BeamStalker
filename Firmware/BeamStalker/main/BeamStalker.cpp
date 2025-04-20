extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#include "esp_console.h"
#include "cmd_system.h"

#include "firmware/helper.h"
#include "firmware/bitmaps.h"
#include "firmware/menu.h"
#include "firmware/interface.h"

#include "firmware/apps/options.h"

#include "firmware/apps/Wifi/beacon_spam.h"
#include "firmware/apps/Wifi/deauther.h"
#include "firmware/apps/Wifi/wifi_main.h"

#include "firmware/apps/BLE/ble_main.h"

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

    while (1) {
        updateBoard();
        if (anyPressed()) {

            if (returnPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
                return 0;
            }
            else if (upPressed()) {
                MainMenuSelector = intChecker(MainMenuSelector - 1, MainMenu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (downPressed()) {
                MainMenuSelector = intChecker(MainMenuSelector + 1, MainMenu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (leftPressed() && (MainMenu.elements[MainMenuSelector].type == 0)) {
                MainMenu.elements[MainMenuSelector].selector = intChecker(MainMenu.elements[MainMenuSelector].selector - 1, MainMenu.elements[MainMenuSelector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (rightPressed()  && (MainMenu.elements[MainMenuSelector].type == 0)) {
                MainMenu.elements[MainMenuSelector].selector = intChecker(MainMenu.elements[MainMenuSelector].selector + 1, MainMenu.elements[MainMenuSelector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }

            else if (selectPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
                switch (MainMenuSelector) {
                    int ret;
                    case 0:  // WiFcker
                        clearScreen();
                        ret = 1;//APP_WiFcker();
                        if (ret != 0) {
                            LogError("Error in app.");
                        }
                        break;
                    case 1:  // BLE
                        clearScreen();
                        ret = APP_BLE();
                        if (ret != 0) {
                            LogError("Error in app.");
                        }
                        break;
                    case 2:  // Options
                        clearScreen();
                        ret = APP_Options();
                        if (ret != 0) {
                            LogError("Error in app.");
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
    /* Prod ready no log at startup*/
    esp_log_level_set("*", ESP_LOG_NONE);

    initBoard();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("BeamStalker %s\n", VERSION);

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = "striker:>";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    /* Loads Modules */
    esp_console_register_help_command();

    register_system();
    module_beaconspam();
    module_deauth();
    module_wifiscan();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#else
#error Unsupported console type
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
