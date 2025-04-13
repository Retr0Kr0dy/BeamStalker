# Wifi
## Beacon Spam

**Description :**

This app simply spam nearby user with AP beacon, thus filling their wifi scanner.

**How it work :**

It either generate a random BSSID with a given charset, or use a specific string and spam [Beacon frame](https://en.wikipedia.org/wiki/Beacon_frame)

**How to use :**

***Arguments***

* **Charset**: Hiragana (あいう), Katakana (アイウ), Cyrillic (ЖБЮ)
* **Start attack**

## Deauther

**Description :**

This app scan for AP, prompt you to select AP and client and then spam forged deauth frame.

**How it work :**

First the AP scan, it's a simple esp wifi scan (dindn't dig too much to see how it work but it's able to get the AP name from Beacon, which i didn't tried).

Then the client scan, it uses the wifi sniffer to catch client for a given AP by sniffing frame and checking if the frame addresses contain both the client and the AP.

```C
    if (is_broadcast(hdr->addr1) || is_broadcast(hdr->addr2) || is_broadcast(hdr->addr3)) {
    } else {
        // For all frames, check if an AP is involved
        for (int i = 0; i < sniff_ap_count; i++) {
            if (mac_equals(hdr->addr1, sniff_ap_list[i].ap_mac.mac)) {
                if (!mac_equals(hdr->addr2, sniff_ap_list[i].ap_mac.mac) && mac_equals(hdr->addr1, hdr->addr3)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr2);
                }
            }
            if (mac_equals(hdr->addr2, sniff_ap_list[i].ap_mac.mac)) {
                if (!mac_equals(hdr->addr1, sniff_ap_list[i].ap_mac.mac) && (memcmp(hdr->addr2, hdr->addr3, 3) == 0)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr1);
                }
            }
            if (mac_equals(hdr->addr3, sniff_ap_list[i].ap_mac.mac)) {
                if (!mac_equals(hdr->addr2, sniff_ap_list[i].ap_mac.mac) && mac_equals(hdr->addr1, hdr->addr3)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr2);
                }
                if (!mac_equals(hdr->addr1, sniff_ap_list[i].ap_mac.mac) && mac_equals(hdr->addr2, hdr->addr3)) {
                    add_client_to_ap(sniff_ap_list[i].ap_mac.mac, hdr->addr1);
                }
            }
        }
    }
```

This method will be enhanced in the future.

**How to use :**

***Arguments***

* **AP mac**: Scan and select with [select_wifi_menu](https://github.com/Retr0Kr0dy/BeamStalker/blob/main/Firmware/BeamStalker/main/firmware/includes/wifi.cpp#L127)
* **Client mac**:  Scan and select with [select_client_menu](https://github.com/Retr0Kr0dy/BeamStalker/blob/main/Firmware/BeamStalker/main/firmware/includes/wifi_sniffer.cpp#L158)
* **Start attack**

## Wifi Sniffer

**Description :**

This app is used to sniff and capture wifi traffic, currently you can only select frame control filter and print packet to serial console, in the future, we should be able to save to pcap file on sdcard

**How it work :**

it sniff packet, check if frame match filter, if so, it log/save the frame

**How to use :**

***Arguments***

* **Filter**: frame control value ([See here](https://en.wikipedia.org/wiki/802.11_frame_types))
* **Start sniffing**