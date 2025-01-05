#ifndef WIFI_MAIN_H
#define WIFI_MAIN_H

#include <M5Cardputer.h>

#include "../../includes/wifi.h"
#include "../../includes/wifi_sniffer.h"
#include "../../menu.h"

#include "app_wifi_sniffer.h"
#include "beacon_spam.h"
#include "deauther.h"

int wifiMenuTask();
int APP_WiFcker();

#endif
