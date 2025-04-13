# Bluetooth

## BLE Spam

**Description :**

This app spam nearby user with BLE advertisement frame (such as airpods pop up).

**How it work :**

It forge frame with predefined devices (for `APPLE`, `SAMSUNG`, `GOOGLE`, `MICROSOFT`) and modify only the sender address each time so the target think it's another device.

> **Tips** ; Apple device spam still work like a charm, same for samsung devices, but for the google device spam, there is a cooldown, it will spawn popup multiple times like 5 or 6, and then nothing, you need to reboot the phone for it to be spammed again

You can also flood bluetooth device scanner using the `NAME` device.

**How to use :**

***Arguments***

* **Devices**: `APPLE`, `SAMSUNG`, `GOOGLE`, `MICROSOFT`, `NAME`, `ALL`
* **Start attack**
