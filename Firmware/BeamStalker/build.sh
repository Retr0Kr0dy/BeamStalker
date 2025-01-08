#!/bin/bash

set -e
set -x

for board in $(ls ./boards);
    do
    config_file=$board
    board_name=${board##*.}
    version=$(grep VERSION main/firmware/helper.h | awk -F'"' '{print $2}')

    cp ./boards/$config_file sdkconfig
    cp ./boards/$config_file sdkconfig.defaults

    idf.py build
    mv ./build/BeamStalker.bin ./bin/BeamStalker-$version-$board_name.bin
    idf.py fullclean

    done

