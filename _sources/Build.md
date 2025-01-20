# Build
This project is designed to be built on any ESP32 chip (adding support for more shouldn’t take too much effort), but it has only been tested on the ESP32 and ESP32-S3 based boards.

Once you have successfully setup your IDE, you can build the project.

> For deauth to work you to do the wsl bypass first. ([See here](https://github.com/Retr0Kr0dy/esp-idf_wsl_bypass))

Clone the project:

```sh
git clone https://github.com/Retr0Kr0dy/BeamStalker
```

Then prepare your project (Here you can choose settings for your specific hardware):

## Single build

Compiling for specific already handled board : 

```sh
cd BeamStalker/Firmwares/BeamStalker # go to the project directory
cp board/sdkconfig.<your-board> sdkconfig.defaults
idf.py build
```
Your firmware should live at `build/BeamStalker.bin`.

## Build all

For github action to build firmware for all board, we made a simple `build.sh` to build firmware for all board confs in `boards/`.

```sh
./build.sh
```

This script will build all boards available at `boards/` and move them to `bin/BeamStaler-<version>-<board-name>.bin`
