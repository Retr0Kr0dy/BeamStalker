#include "interface.h"

bool DEFAULT_BTN_PRESSED,
    DEFAULT_BTN_FAST_PRESS,
    DEFAULT_BTN_LONG_PRESS,
    DEFAULT_BTN_DOUBLE_PRESS,
    DEFAULT_BTN_FAST_LONG_PRESS = false;
    
bool DEFAULT_BTN_LAST_WAS_PRESSED = false;

int handleDefaultButton() { // Need to be enhanced a lot
    DEFAULT_BTN_FAST_PRESS = false;
    DEFAULT_BTN_LONG_PRESS = false;
    DEFAULT_BTN_DOUBLE_PRESS = false;
    DEFAULT_BTN_FAST_LONG_PRESS = false;

    if (gpio_get_level(DEFAULT_BTN)||DEFAULT_BTN_LAST_WAS_PRESSED) {
        DEFAULT_BTN_PRESSED = false;
        DEFAULT_BTN_LAST_WAS_PRESSED = false;
        return 0; // not pressed so exit
    }

    DEFAULT_BTN_PRESSED = true;

    time_t start_time = time(NULL);
    time_t end_time = start_time + 1;
    
    bool previous_state = gpio_get_level(DEFAULT_BTN);
    int press_count = 0;
    int release_count = 0;

    while (time(NULL) < end_time) {
        bool current_state = gpio_get_level(DEFAULT_BTN);
        if (current_state != previous_state) {
            if (current_state == 1) {
                press_count++;
            } else {
                release_count++;
            }
            previous_state = current_state;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }

    if (!gpio_get_level(DEFAULT_BTN)) {
        DEFAULT_BTN_LAST_WAS_PRESSED = true;
    }

    if (press_count == 1 && release_count == 0) {
        DEFAULT_BTN_FAST_PRESS = true;  // Fast press
    } else if (press_count == 0 && release_count == 0 && DEFAULT_BTN_LAST_WAS_PRESSED) {
        DEFAULT_BTN_LONG_PRESS = true;  // Long press
    } else if (press_count == 2 && release_count == 1) {
        DEFAULT_BTN_DOUBLE_PRESS = true;  // Double press
    } else if (press_count == 1 && release_count == 1 && DEFAULT_BTN_LAST_WAS_PRESSED) {
        DEFAULT_BTN_FAST_LONG_PRESS = true;  // Fast Long press
    }

    return 0;
}

bool upPressed() {
    #ifdef CONFIG_M5_BOARD
    return M5Cardputer.Keyboard.isKeyPressed(';');
    #else
    return false;
    #endif
}
bool downPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_DOUBLE_PRESS;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed('.');
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}
bool leftPressed() {
    #ifdef CONFIG_M5_BOARD
    return M5Cardputer.Keyboard.isKeyPressed(',');
    #else
    return false;
    #endif
}
bool rightPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_FAST_LONG_PRESS;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed('/');
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}
bool selectPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_FAST_PRESS;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}
bool returnPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_LONG_PRESS;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed('`');
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}

bool anyPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;
    
    default_btn = DEFAULT_BTN_PRESSED;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isPressed();
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}

void updateBoard() {
    handleDefaultButton();
    #ifdef CONFIG_M5_BOARD
    M5Cardputer.update();
    #endif
}

void initBoard() {
    gpio_config_t default_btn_conf = {
        .pin_bit_mask = (1ULL << DEFAULT_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&default_btn_conf);

    #ifdef CONFIG_M5_BOARD
    M5Cardputer.begin(true);
    M5.Power.begin();

    M5.Display.setTextSize(charsize_multiplier);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::FreeMonoBold18pt7b);
    #endif
}

int getBatteryLevel() {
    #ifdef CONFIG_M5_BOARD
    return M5Cardputer.Power.getBatteryLevel();
    #else
    return -1;
    #endif
}

int16_t getBatteryVoltage() {
    #ifdef CONFIG_M5_BOARD
    return M5Cardputer.Power.getBatteryVoltage();
    #else
    return -1;
    #endif    
}

int isCharging() {
    #ifdef CONFIG_M5_BOARD
    return M5Cardputer.Power.isCharging();
    #else
    return -1;
    #endif    
}

void displayText(int x, int y, const char* text, uint32_t color) {
    #ifdef CONFIG_M5_BOARD
    M5.Display.setCursor(x, y);
    M5.Display.setTextColor(color);
    M5.Display.print(text);
    #endif
}

void clearScreen(uint32_t color) {
    #ifdef CONFIG_M5_BOARD
    M5.Display.fillScreen(color);
    #endif
}

void drawPixel(int32_t x, int32_t y, const unsigned int &color) {
    #ifdef CONFIG_M5_BOARD
    M5.Display.drawPixel(x, y, color);
    #endif
}

void drawFillRect(int x, int y, int end_x, int end_y, const unsigned int &color) {
    #ifdef CONFIG_M5_BOARD
    M5.Display.fillRect(x, y, end_x, end_y, color);
    #endif
}