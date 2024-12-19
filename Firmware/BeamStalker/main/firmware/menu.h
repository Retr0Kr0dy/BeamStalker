#ifndef MENU_H
#define MENU_H

#include <M5Cardputer.h>

#define MAX_OPTIONS 10
#define MAX_NAME_LENGTH 14

struct item {
//    const char *name;
    char name[MAX_NAME_LENGTH];
    int type; // 0: Has options, 1: No options
    int length;
    const char *options[MAX_OPTIONS];
    int selector;
};

struct menu {
    const char *name;
    struct item *elements;
    int length;
};

float charsize_multiplier = 0.5;
int font_size = 18;
int charsize = (int)(font_size*charsize_multiplier)+((40*font_size)/100);

int intChecker(int value, int length);
char *createHeaderLine(const char *menu_name);
void drawMenu(struct menu Menu, int selector);
void M5GFX_display_text(int x, int y, const char* text, uint32_t color = TFT_WHITE);
void M5GFX_clear_screen(uint32_t color = TFT_BLACK);
int LogError(const std::string& message);
void drawBitmap(int16_t x, int16_t y, int16_t width, int16_t height, const uint8_t *bitmap, uint32_t color);

#endif