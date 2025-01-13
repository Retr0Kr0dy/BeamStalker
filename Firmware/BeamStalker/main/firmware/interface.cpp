#include "interface.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "rom/uart.h"

bool DEFAULT_BTN_PRESSED,
    DEFAULT_BTN_FAST_PRESS,
    DEFAULT_BTN_LONG_PRESS,
    DEFAULT_BTN_DOUBLE_PRESS,
    DEFAULT_BTN_FAST_LONG_PRESS = false;
    
bool DEFAULT_BTN_LAST_WAS_PRESSED = false;

bool SERIAL_CONSOLE_UP,
    SERIAL_CONSOLE_DOWN,
    SERIAL_CONSOLE_LEFT,
    SERIAL_CONSOLE_RIGHT,
    SERIAL_CONSOLE_SELECT,
    SERIAL_CONSOLE_RETURN = false;

bool SERIAL_CONSOLE_NOT_EMPTY = false;

#define UART_PORT_NUM    UART_NUM_0
#define BUF_SIZE         1024

int checkUartChar() {
    uint8_t data[BUF_SIZE];
    
    //bool ret = uart_rx_one_char(reinterpret_cast<uint8_t*>(&chr));
    int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_RATE_MS);
    if (len > 0 && len < BUF_SIZE) {
        char chr = data[0];
        SERIAL_CONSOLE_NOT_EMPTY = true;
        SERIAL_CONSOLE_UP = (chr == 'z');
        SERIAL_CONSOLE_DOWN = (chr == 's');
        SERIAL_CONSOLE_LEFT = (chr == 'q');
        SERIAL_CONSOLE_RIGHT = (chr == 'd');
        SERIAL_CONSOLE_RETURN = (chr == 'a');
        SERIAL_CONSOLE_SELECT = (chr == 'e');
    } else {
        SERIAL_CONSOLE_NOT_EMPTY = false;
    }

    return len;
}

bool check_uart_input(const char *str) {
    uint8_t data[BUF_SIZE];
    int length = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, 0);
    if (length > 0) {
        data[length] = '\0';
        printf("Received data: %s", data);

        SERIAL_CONSOLE_NOT_EMPTY = true;

        if (strstr((char *)data, str)) {
            return true;
        }
    }
    SERIAL_CONSOLE_NOT_EMPTY = false;
    return false;
}

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
    bool default_btn = false, custom_btn = false, serial_btn = false;

    serial_btn = SERIAL_CONSOLE_UP;
    
    #ifdef CONFIG_M5_BOARD
    return M5Cardputer.Keyboard.isKeyPressed(';');
    #else
    return false;
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}
bool downPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_DOUBLE_PRESS;
    serial_btn = SERIAL_CONSOLE_DOWN;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed('.');
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}
bool leftPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    serial_btn = SERIAL_CONSOLE_LEFT;

    #ifdef CONFIG_M5_BOARD
    return M5Cardputer.Keyboard.isKeyPressed(',');
    #else
    return false;
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}

bool rightPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_FAST_LONG_PRESS;
    serial_btn = SERIAL_CONSOLE_RIGHT;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed('/');
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}
bool selectPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_FAST_PRESS;
    serial_btn = SERIAL_CONSOLE_SELECT;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}
bool returnPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;

    default_btn = DEFAULT_BTN_LONG_PRESS;
    serial_btn = SERIAL_CONSOLE_RETURN;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isKeyPressed('`');
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}

bool anyPressed() {
    bool default_btn = false, custom_btn = false, serial_btn = false;
    
    default_btn = DEFAULT_BTN_PRESSED;
    serial_btn = SERIAL_CONSOLE_NOT_EMPTY;

    #ifdef CONFIG_M5_BOARD
    custom_btn = M5Cardputer.Keyboard.isPressed();
    #endif

    return (bool)(default_btn||custom_btn||serial_btn);
}

void updateBoard() {
    handleDefaultButton();
    
    #ifndef CONFIG_M5_BOARD // keeping it like at the moment to narrow the bug (not working only on cardputer)
    checkUartChar();
    #endif
    #ifdef CONFIG_M5_BOARD
    M5Cardputer.update();
    #endif
}

void initBoard() {
    // defauilt button
    gpio_config_t default_btn_conf = {
        .pin_bit_mask = (1ULL << DEFAULT_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&default_btn_conf);

    // serial console
    #ifndef CONFIG_M5_BOARD
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(UART_PORT_NUM, &uart_config);
 
    esp_err_t err = uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        printf("Failed to install UART driver: %s\n", esp_err_to_name(err));
        return;
    }
    #endif

    #ifdef CONFIG_M5_BOARD
    M5Cardputer.begin(true);
    M5.Power.begin();
    M5.Display.setTextSize(charsize_multiplier);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::FreeMonoBold18pt7b);
    #elif CONFIG_HELTEC_BOARD
    i2c_master_init(&display, CONFIG_SCL_GPIO, CONFIG_SDA_GPIO, CONFIG_RESET_GPIO);
    ssd1306_init(&display, 128, 64);
    ssd1306_contrast(&display, 0xff);
    ssd1306_clear_screen(&display, false);
    #endif
    #ifdef CONFIG_HAS_SDCARD
    initSDCard();    



            // testSDCard();



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
    M5.Display.setCursor(x, y*charsize);
    M5.Display.setTextColor(color);
    M5.Display.print(text);
    #elif CONFIG_HELTEC_BOARD
    ssd1306_display_text(&display, y, (char*)text, strlen(text), false);
    #endif
}

void clearScreen(uint32_t color) {
    #ifdef CONFIG_M5_BOARD
    M5.Display.fillScreen(color);
    #elif CONFIG_HELTEC_BOARD
    ssd1306_clear_screen(&display, false);
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

int LogError(const std::string& message) {
    clearScreen();
    displayText(0, 1, message.c_str(), TFT_RED);
    vTaskDelay(pdMS_TO_TICKS(5000));
    return 0;
}

void drawBitmap(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *bitmap, uint32_t color) {
    #ifdef CONFIG_HELTEC_BOARD
    ssd1306_bitmaps(&display, x, y, (uint8_t*)bitmap, width, height, false);
    #else
    for (int16_t i = 0; i < height; i++) {
        for (int16_t j = 0; j < width; j++) {
            uint8_t bit = (bitmap[i * (width / 8) + (j / 8)] >> (7 - (j % 8))) & 1;
            drawPixel(x + j, y + i, bit ? color : TFT_BLACK);
        }
    }
    #endif
}