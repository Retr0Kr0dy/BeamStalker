#ifdef CONFIG_HAS_SDCARD

#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdio>
extern "C" {
#include "../sd_card.h"
}
#include "../menu.h"

int APP_Notepad(const char* initPath);

#endif
#endif