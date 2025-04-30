![](https://github.com/Retr0Kr0dy/BeamStalker/blob/main/assets/beamstalker-dark-background.png)

BeamStalker is open-source firmware for esp based boards designed for RF experimentation, encouraging collaboration and ethical exploration. No profit motive here—just a project for those who want to mess around with RF tech and hack their way through.

Open-source means you can poke around, modify, and make it your own. It’s all about collaboration and sharing your findings with the community. Explore, experiment, and push boundaries, but keep it ethical—don’t bring any malicious nonsense into it. 

Join in, share your findings, and let’s see what we can create together. Happy hacking!

##### Knowledge should be free 🏴‍☠️

## 🛠️ Installation

Currently you have two choices, either you get the latest bin file for you board in the [release section](https://github.com/Retr0Kr0dy/BeamStalker/releases) and flash it to your board, or you can build the project from source using esp-idf v4.4 (see [Get Started](https://retr0kr0dy.github.io/BeamStalker/Setup.html)).

> [!NOTE]
> Work gracefully with the [M5Launcher project](https://github.com/bmorcelli/M5Stick-Launcher).

## 📖 Usage

This project is mainly a CLI giving utlra modular capabilities inspired by this [Shell-ESP32](https://github.com/Eun0us/Shell-ESP32) project.

For more specific usage docs, go check the [wiki](https://retr0kr0dy.github.io/BeamStalker).

## ✨ Features

|Type|Name|Description|Status|
|-|-|-|-|
|Wifi|BeaconSpam|Spam nearby client with dumb wifi AP|Fully implemented|
|Wifi|Deauther|Spam either boadcast, or targeted client with deauth frame|Fully implemented|
|Wifi|WifiSniffer|Sniff 802.11 frame accroding to a given filter|Fully Implemented|
|BLE|BleSpam|Spam nearby client with ble adv frames|Fully implemented|
|RF|RecordReplay|Record and replay subghz frame|Implementing|
|RF|TPMS|Forge and patch tpms frame|To do|

## 🖥️ Supported boards

- [esp32](https://www.espressif.com/en/products/socs/esp32)
- [esp32s3](https://www.espressif.com/en/products/socs/esp32s3)
> - [M5Cardputer](https://shop.m5stack.com/products/m5stack-cardputer-kit-w-m5stamps3) (not on V0.3)

> - [HeltecV3](https://heltec.org/project/wifi-lora-32-v3/) (not on V0.3)

## 🙏 Acknowledgements / other cool projects

- [@sdardouchi](https://github.com/sdardouchi) for the github pages (go check [OperationSE](https://github.com/OperationSE) if you like old school phone hack) 
- [@Eun0us](https://github.com/Eun0us) for the incredible research work on the [esp console](https://github.com/Eun0us/Shell-ESP32)
- [@bmorcelli](https://github.com/bmorcelli) for his genius [M5Launcher](https://github.com/bmorcelli/Launcher) and work on [Bruce Firmware](https://github.com/pr3y/Bruce)
- [@pr3y](https://github.com/pr3y) also for [Bruce Firmware](https://github.com/pr3y/Bruce)
- [@7h30th3r0n3](https://github.com/7h30th3r0n3) for the [Evil-M5Project](https://github.com/7h30th3r0n3/Evil-M5Project)
- [@risinek](https://github.com/risinek/) for the [pcap serializer](https://github.com/risinek/esp32-wifi-penetration-tool/tree/master/components/pcap_serializer)
- [@Spacehuhn](https://github.com/SpacehuhnTech/esp8266_deauther) for the [deauth bypass](https://github.com/SpacehuhnTech/esp8266_deauther) on esp
- and all other projects that I took inspiration from; knowledge should be free 🏴‍☠️.
---
*Made with fun by akpalanaza*

