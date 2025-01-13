# BeamStalker

BeamStalker is open-source firmware for esp based board designed for RF experimentation, encouraging collaboration and ethical exploration. No profit motive here—just a project for those who want to mess around with RF tech and hack their way through.

Open-source means you can poke around, modify, and make it your own. It’s all about collaboration and sharing your findings with the community. Explore, experiment, and push boundaries, but keep it ethical—don’t bring any malicious nonsense into it. 

Join in, share your findings, and let’s see what we can create together. Happy hacking!

## Installation

Currently you have two choices, either you get the latest bin file for you board in the [release section](https://github.com/Retr0Kr0dy/BeamStalker/releases) and flash it to your board, or you can build the project from source using esp-idf v4.4 (see [build](https://github.com/Retr0Kr0dy/BeamStalker/wiki/Build)).

> [!NOTE]
> Work gracefully with the [M5Launcher project](https://github.com/bmorcelli/M5Stick-Launcher).

## Usage

You got 3 main way to control the firmware :

- Main one, using custom gpio, you will need to have a board with button, but it's the intend way of usage.
- Second one, is using the main button on esp (short press, long press, ...) but it's currently buggy.
- Third one, is using the serial console, but this imply that you have a phone or computer nearby to control.

For more specific usage docs, go check the [wiki](https://github.com/Retr0Kr0dy/BeamStalker/wiki).

## Features

|Type|Name|Description|Status|
|-|-|-|-|
|Wifi|BeaconSpam|Spam nearby client with dumb wifi AP|Fully implemented|
|Wifi|Deauther|Spam either boadcast, or targeted client with deauth frame|Fully implemented|
|Wifi|WifiSniffer|Sniff 802.11 frame accroding to a given filter|Implementing|
|BLE|BleSpam|Spam nearby client with ble adv frames|Fully implemented|
|RF|RecordReplay|Record and replay subghz frame|To do|
|RF|TPMS|Forge and patch tpms frame|To do|

## Supported boards

- [esp32](https://www.espressif.com/en/products/socs/esp32)
- [M5Cardputer](https://shop.m5stack.com/products/m5stack-cardputer-kit-w-m5stamps3)
- [HeltecV3](https://heltec.org/project/wifi-lora-32-v3/)

---
*Made with fun by akpalanaza*

