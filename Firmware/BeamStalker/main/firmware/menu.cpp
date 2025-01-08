#include "menu.h"

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

    auto chargingStatus = isCharging();
    char is_charging_str[4];

    if (chargingStatus == true) {
        snprintf(is_charging_str, sizeof(is_charging_str), "CHG"); // if charging
    } else if (chargingStatus == false) {
        snprintf(is_charging_str, sizeof(is_charging_str), "   "); // if not charging, fill with spaces
    } else {
        snprintf(is_charging_str, sizeof(is_charging_str), "UNK"); // Unknown status
    }

    int batteryLevel = getBatteryLevel();
    char bat_percentage_str[12];

    if (batteryLevel >= 0 && batteryLevel <= 100) {
        snprintf(bat_percentage_str, sizeof(bat_percentage_str), "%d%%", batteryLevel);
    } else {
        snprintf(bat_percentage_str, sizeof(bat_percentage_str), "N/A");
    }

    int16_t batteryVoltage = getBatteryVoltage();
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

void serialMenu(struct menu Menu, int selector) {
    printf ("-=%s=-\n",Menu.name);

    for (int i = 0; i < Menu.length; i++) {
        if (selector == i) {printf (">");}
        if (selector == i) {printf (" ");}
        printf ("%d - %s\n", i, Menu.elements[i].name);
        if (Menu.elements[i].type == 1) {
            for (int j = 0; j < Menu.elements[i].length; j++) {
                printf (" -< %d%d: %s\n",i,j, Menu.elements[i].options[j]);
            }
        }
    }
}

void drawMenu(struct menu Menu, int selector) {
    serialMenu(Menu, selector);
    char fullMenuName[50];
    sprintf(fullMenuName, "%s",createHeaderLine(Menu.name));

    clearScreen();

    drawFillRect(0, 0, DISPLAY_WIDTH, charsize, TFT_WHITE);
    displayText(0, 0, fullMenuName, TFT_BLACK);

    int j = 1;
    char element_str[50];

    for (int i = -3; i <= 3; i++) {
        struct item element = Menu.elements[intChecker(selector+i, Menu.length)];

        if (element.type == 0) {
            const int max_element = 14;
            const int max_value = 10-4;
            char cropped_element[max_element + 1];

            if (strlen(element.name) > max_element) {
                strncpy(cropped_element, element.name + (strlen(element.name) - max_element), max_element);
                cropped_element[max_element] = '\0';
            } else {
                strncpy(cropped_element, element.name, max_element);
                cropped_element[strlen(element.name)] = '\0';
            }

            if (i == 0) {
                snprintf(element_str, sizeof(element_str), "%-*s <%-*s>", max_element, cropped_element, max_value, element.options[intChecker(element.selector, element.length)]);
            } else {
                snprintf(element_str, sizeof(element_str), "%-*s  %-*s ", max_element, cropped_element, max_value, element.options[intChecker(element.selector, element.length)]);
            }
        } else if (element.type == 1) {
            snprintf (element_str, sizeof(element_str), "%s",element.name);
        }

        if (i == 0) {
            displayText(0, j*charsize, element_str, TFT_CYAN);
        } else {
            displayText(0, j*charsize, element_str, TFT_WHITE);
        }
        j++;
    }
}

int LogError(const std::string& message) {
    clearScreen();
    displayText(0, 1 * charsize, message.c_str(), TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(5000));
    return 0;
}

void drawBitmap(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *bitmap, uint32_t color) {
    for (int16_t i = 0; i < height; i++) {
        for (int16_t j = 0; j < width; j++) {
            uint8_t bit = (bitmap[i * (width / 8) + (j / 8)] >> (7 - (j % 8))) & 1;
            drawPixel(x + j, y + i, bit ? color : TFT_BLACK);
        }
    }
}
