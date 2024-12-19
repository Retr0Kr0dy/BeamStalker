# BeamStalker
Some clandestine RF toy.

### Currently 2 firmware version :

- [v0.1](https://github.com/Retr0Kr0dy/BeamStalker/tree/v0.1) for ESP32 common.

- [v0.2](https://github.com/Retr0Kr0dy/BeamStalker/tree/v0.2) for M5Cardputer.


### For M5Cardputer

Please use esp-idf version 4.4 as it's currently the only one where `M5.Power.getBatteryLevel()` is working, else on esp-idf version 5, lib doesn't seem to work, still investigating.

#### Usage

Using esp-idf 4.4 :

```sh
cd firmwares/BeamStalker # go to the project directory
idf.py add-dependency "m5stack/m5unified" # Required for M5Cardputer lib
idf.py set-target esp32s3 # For M5 StampS3
idf.py menuconfig
```

In menuconfig, we need to :

- 8MB flash size (Main -> Serial flasher config)
- Use custom partition.csv (Main -> Partition Table -> Partition Table)

Then you can build the project : 

```sh
idf.py build
```

Can work with the [M5Launcher project](https://github.com/bmorcelli/M5Stick-Launcher)

> [!IMPORTANT]
> Needed for the wsl bypass to work. ([See here](https://github.com/Retr0Kr0dy/esp-idf_wsl_bypass))

---
*Made with fun by akpalanaza*

