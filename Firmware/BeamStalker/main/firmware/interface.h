#ifndef INTERFACE_H
#define INTERFACE_H

#include "driver/gpio.h"
#include <string>

#include "menu.h"

#ifdef CONFIG_M5_BOARD
#include <M5Cardputer.h>
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
#endif

#endif