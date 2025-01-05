#!/bin/bash

set -e
set -x

# Check if ESP-IDF environment variables are set
if [[ -z "$IDF_PATH" ]]; then
    echo "ESP-IDF is not set up. Please source the ESP-IDF export script (e.g., . ./export.sh)."
    exit 1
fi



# Define paths for the required libraries and tools
esp32_file="$IDF_PATH/components/esp_wifi/lib/esp32/libnet80211.a"
esp32s3_file="$IDF_PATH/components/esp_wifi/lib/esp32s3/libnet80211.a"

esp32_file_temp="$IDF_PATH/components/esp_wifi/lib/esp32/libnet80211_temp.a"
esp32s3_file_temp="$IDF_PATH/components/esp_wifi/lib/esp32s3/libnet80211_temp.a"

# Find the objcopy tools dynamically

# Use $IDF_TOOLS_PATH if defined, otherwise fall back to $HOME/.espressif/tools
tools_path="${IDF_TOOLS_PATH:-$HOME/.espressif}/tools"

toolchain_esp32=$(find "$tools_path" -type f -name "xtensa-esp32-elf-objcopy" | head -n 1)
toolchain_esp32s3=$(find "$tools_path" -type f -name "xtensa-esp32s3-elf-objcopy" | head -n 1)

# Verify the existence of the required library files
if [[ -f "$esp32_file" && -f "$esp32s3_file" ]]; then
    echo "Required library files found. Proceeding with modifications..."

    # Execute objcopy for ESP32
    $toolchain_esp32 --weaken-symbol=ieee80211_raw_frame_sanity_check "$esp32_file" "$esp32_file_temp"

    # Rename the original file to .old
    mv "$esp32_file" "${esp32_file}.old"

    # Rename the _temp file to the original name
    mv "$esp32_file_temp" "$esp32_file"

    echo "Modified ESP32 libnet80211.a"

    # Execute objcopy for ESP32-S3
    $toolchain_esp32s3 --weaken-symbol=ieee80211_raw_frame_sanity_check "$esp32s3_file" "$esp32s3_file_temp"

    # Rename the original file to .old
    mv "$esp32s3_file" "${esp32s3_file}.old"

    # Rename the _temp file to the original name
    mv "$esp32s3_file_temp" "$esp32s3_file"

    echo "Modified ESP32-S3 libnet80211.a"

    echo "All modifications completed successfully."
else
    echo "One or more specified files were not found. Ensure ESP-IDF is correctly set up and the files exist in the expected paths."
    exit 1
fi

