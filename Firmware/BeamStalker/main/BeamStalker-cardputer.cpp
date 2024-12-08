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

#include "firmware/apps/Options.h"
#include "firmware/apps/wifcker.h"

#include "M5GFX.h"
#include "M5Cardputer.h"

M5GFX display;
M5Canvas canvas(&display);

float charsize_multiplier = 0.5;
int font_size = 18;
int charsize = (int)(font_size*charsize_multiplier)+((40*18)/100);

int LogError(const std::string& message) {
    M5GFX_clear_screen();
    M5GFX_display_text(0, 1 * charsize, message.c_str(), TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(5000));
    return 0;
}

void drawBitmap(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *bitmap, uint32_t color) {
    for (int16_t i = 0; i < height; i++) {
        for (int16_t j = 0; j < width; j++) {
            uint8_t bit = (bitmap[i * (width / 8) + (j / 8)] >> (7 - (j % 8))) & 1;
            M5.Display.drawPixel(x + j, y + i, bit ? color : TFT_BLACK);
        }
    }
}

int intChecker(int value, int length) {
    while (value < 0) {
        value += length;
    }
    while (value > length - 1) {
        value -= length;
    }
    return value;
}

char *createHeaderLine(const char *menu_name) {
    const int max_menu_name = 10 + 7 - 6;

    static char final[50];
    char cropped_menu_name[max_menu_name + 1];

    if (strlen(menu_name) > max_menu_name) {
        strncpy(cropped_menu_name, menu_name + (strlen(menu_name) - max_menu_name), max_menu_name);
        cropped_menu_name[max_menu_name] = '\0';
    } else {
        strncpy(cropped_menu_name, menu_name, max_menu_name);
        cropped_menu_name[strlen(menu_name)] = '\0';
    }

    auto chargingStatus = M5Cardputer.Power.isCharging();
    char is_charging_str[4];

    if (chargingStatus == true) {
        snprintf(is_charging_str, sizeof(is_charging_str), "CHG"); // if charging
    } else if (chargingStatus == false) {
        snprintf(is_charging_str, sizeof(is_charging_str), "   "); // if not charging, fill with spaces
    } else {
        snprintf(is_charging_str, sizeof(is_charging_str), "UNK"); // Unknown status
    }

    int batteryLevel = M5Cardputer.Power.getBatteryLevel();

    char bat_percentage_str[12];

    if (batteryLevel >= 0 && batteryLevel <= 100) {
        snprintf(bat_percentage_str, sizeof(bat_percentage_str), "%d%%", batteryLevel);
    } else {
        snprintf(bat_percentage_str, sizeof(bat_percentage_str), "N/A");
    }

    int16_t batteryVoltage = M5Cardputer.Power.getBatteryVoltage();

    char bat_voltage_str[12];

    if (batteryVoltage >= 0 && batteryVoltage <= 10000) {
        float batVoltage = batteryVoltage / 1000.0f;
        snprintf(bat_voltage_str, sizeof(bat_voltage_str), "%.1fV", (batVoltage));
    } else {
        snprintf(bat_voltage_str, sizeof(bat_voltage_str), "N/A");
    }

    snprintf(final, sizeof(final), "%-*s %s %s %s", max_menu_name, cropped_menu_name, is_charging_str, bat_percentage_str, bat_voltage_str);
    return final;
}

void drawMenu(struct menu Menu, int selector) {
    char fullMenuName[50];
    sprintf(fullMenuName, "%s",createHeaderLine(Menu.name));

    M5GFX_clear_screen();

    M5.Display.fillRect(0, 0, M5.Display.width(), charsize, TFT_CYAN);
    M5GFX_display_text(0, 0, fullMenuName, TFT_BLACK);

    int j = 1;
    char element_str[50];

    for (int i = -3; i <= 3; i++) {
        struct item element = Menu.elements[intChecker(selector+i, Menu.length)];

        if (element.type == 0) {
            snprintf (element_str, sizeof(element_str), "%s: %s", element.name, element.options[intChecker(element.selector, element.length)]);
        } else if (element.type == 1) {
            snprintf (element_str, sizeof(element_str), "%s",element.name);
        }

        if (i == 0) {
            M5GFX_display_text(0, j*charsize, element_str, TFT_GREEN);
        } else {
            M5GFX_display_text(0, j*charsize, element_str, TFT_WHITE);
        }
        j++;
    }
}

int mainTask() {
    vTaskDelay(pdMS_TO_TICKS(100));

    int MainMenuSelector = 0;

    struct menu MainMenu;

    MainMenu.name = "~/";
    MainMenu.length = 3;  // WiF, Eye, Opt
    MainMenu.elements = new item[MainMenu.length];

    MainMenu.elements[0].name = "WiFcker";
    MainMenu.elements[0].type = 1;
    MainMenu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        MainMenu.elements[0].options[i] = NULL;
    }

    MainMenu.elements[1].name = "The Eye";
    MainMenu.elements[1].type = 0;
    MainMenu.elements[1].length = 2;
    MainMenu.elements[1].options[0] = "opt1";
    MainMenu.elements[1].options[1] = "opt2";
    MainMenu.elements[1].options[2] = NULL; // Terminate options explicitly


    MainMenu.elements[2].name = "Options";
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
                    case 2:  // Options
                        M5GFX_clear_screen();
                        ret = APP_Options();
                        if (ret != 0) {
                            printf("Error in app.");
                        }
                        break;
                    case 0:  // WiFcker
                        M5GFX_clear_screen();
                        ret = APP_WiFcker();
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
        int16_t x = (M5.Display.width() - 128) / 2;
        drawBitmap(x, 0, 128, 64, skullLogo, TFT_WHITE);
        vTaskDelay(pdMS_TO_TICKS(1000));

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
