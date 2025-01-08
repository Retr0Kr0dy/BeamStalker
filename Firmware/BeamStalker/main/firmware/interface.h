#ifndef INTERFACE_H
#define INTERFACE_H


#include "esp_event.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "time.h"
#include "driver/gpio.h"

#include <string>

#ifndef CONFIG_M5_BOARD
#include "string.h"
#endif

#include "menu.h"

#ifdef CONFIG_M5_BOARD
#include <M5Cardputer.h>
#endif

// Temp fix for missing M5Cardputer.h
#ifndef CONFIG_M5_BOARD
#undef TFT_WHITE
#undef TFT_BLACK
#undef TFT_RED
static constexpr int TFT_WHITE = 0xFFFF;
static constexpr int TFT_BLACK = 0x0000;
static constexpr int TFT_RED = 0x0101;
static constexpr int TFT_CYAN = 0x0101;
#endif

#define DEFAULT_BTN GPIO_NUM_0

bool upPressed();
bool downPressed();
bool leftPressed();
bool rightPressed();
bool selectPressed();
bool returnPressed();
bool anyPressed();
void updateBoard();
void initBoard();
int getBatteryLevel();
int16_t getBatteryVoltage();
int isCharging();

void displayText(int x, int y, const char* text, uint32_t color = TFT_WHITE);
void clearScreen(uint32_t color = TFT_BLACK);
void drawPixel(int32_t x, int32_t y, const unsigned int &color = TFT_WHITE);
void drawFillRect(int x, int y, int end_x, int end_y, const unsigned int &color = TFT_WHITE);

#ifdef CONFIG_M5_BOARD
M5GFX display;
M5Canvas canvas(&display);
#define DISPLAY_WIDTH M5.Display.width()
#define DISPLAY_HEIGHT M5.Display.height()
#else
#define DISPLAY_WIDTH 13
#define DISPLAY_HEIGHT 37
#endif

#endif