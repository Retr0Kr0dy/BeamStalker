# Setup
Right now, the project is built using esp-idf, so you'll either want to work with that or tweak it to fit your IDE.

You can build the whole project with `esp-idf >= v4 (tested)` , but if you're using M5Stack hardware, you'll need to stick with `esp-idf < v5`. It seems that `M5.Power.getBatteryLevel()` and some other features only play nice with versions up to 4.4 (that's what I've tested).

## Install esp-idf v4.4

Simply clone the repo on the correct branch and install it:

```sh
git clone -b v4.4 --single-branch https://github.com/espressif/esp-idf
./install.sh
. ./export.sh # you should use this on any new env
```
[See for more information](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html)

## WSL Bypass

To send the deauth packet, we need to bypass the `ieee80211_raw_frame_sanity_check()` function, as it blocks frames that start with `0x0C`.

To get around this, we’ll modify the `libnet80211.a` file in esp-idf and give the `ieee80211_raw_frame_sanity_check()` function a `weak` attribute. 

This way, we can redefine the function later in our firmware.

[See for more information](https://github.com/Retr0Kr0dy/esp-idf_wsl_bypass)

