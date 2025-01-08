#ifndef BEACON_SPAM_H
#define BEACON_SPAM_H

#include "../../includes/wifi.h"
#include "../../menu.h"

int charset;

void send_beacon(const char *ssid, const uint8_t *mac_addr, uint8_t channel);
char *generate_ssid();
void trollBeacon();
int BeaconSpam();

#endif 