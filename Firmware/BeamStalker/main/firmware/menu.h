#ifndef MENU_H
#define MENU_H

#include "interface.h"

#define MAX_OPTIONS 10
#define MAX_NAME_LENGTH 14

struct item {
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

int intChecker(int value, int length);
char *createHeaderLine(const char *menu_name);
void drawMenu(struct menu Menu, int selector);
#endif