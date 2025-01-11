#ifndef SD_CARD_H
#define SD_CARD_H

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>


#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#define MOUNT_POINT "/sdcard"

sdmmc_card_t *card;

#define PIN_NUM_MOSI CONFIG_PIN_MOSI
#define PIN_NUM_MISO CONFIG_PIN_MISO
#define PIN_NUM_CLK CONFIG_PIN_CLK
#define PIN_NUM_CS CONFIG_PIN_CS

// void testSDCard();

void initSDCard();
void closeSDCard();

esp_err_t s_write_file(const char *path, const char *data);
esp_err_t s_read_file(const char *path);
esp_err_t s_list_dir(const char *path);
esp_err_t s_create_dir(const char *path);
esp_err_t s_remove_dir(const char *path);
void trim_path(char *path);
bool is_directory_empty(const char *path);
esp_err_t s_rename_dir(const char *old_path, const char *new_path);
void s_read_bytes_from_file(const char *filename, uint8_t *buffer, size_t size);
esp_err_t s_write_bytes_to_file(const char *path, const uint8_t *data, size_t size);
long long get_file_size(const char *filename);
char *get_full_path(const char *base, const char *filename);

#endif