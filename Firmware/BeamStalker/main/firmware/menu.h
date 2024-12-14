#ifndef MENU_H
#define MENU_H

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

#endif
