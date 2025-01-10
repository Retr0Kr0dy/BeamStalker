#ifndef SD_CARD_H
#define SD_CARD_H

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#define MOUNT_POINT "/BeamStalker"

sdmmc_card_t *card;

#define PIN_NUM_MOSI CONFIG_PIN_MOSI
#define PIN_NUM_MISO CONFIG_PIN_MISO
#define PIN_NUM_CLK CONFIG_PIN_CLK
#define PIN_NUM_CS CONFIG_PIN_CS

void initSDCard();

#endif