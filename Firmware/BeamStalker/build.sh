#!/bin/bash

set -x
set -e

for board in $(ls ./boards);
    do
    config_file=$board
    board_name=${board##*.}
    version=$(grep VERSION main/firmware/helper.h | awk -F'"' '{print $2}')
    mcu=$(grep "CONFIG_IDF_TARGET" boards/$board | awk -F'"' '{print $2}')

    if [ -z "$IDF_TARGET" ]; then
        echo "IDF_TARGET is not set. Compiling anyway..."
    elif [ "$IDF_TARGET" != "$mcu" ]; then
        echo "IDF_TARGET ($IDF_TARGET) does not match MCU ($mcu). Next candidate"
        continue
    fi

    idf.py set-target $mcu

    cp ./boards/$config_file sdkconfig
    cp ./boards/$config_file sdkconfig.defaults

    idf.py build
    mv ./build/BeamStalker.bin ./bin/BeamStalker-$version-$board_name.bin
    idf.py fullclean
    rm sdkconfig sdkconfig.defaults
    done

