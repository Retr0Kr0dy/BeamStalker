#ifndef MENU_H
#define MENU_H

#define MAX_OPTIONS 10

struct item {
    const char *name;
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

#endif
