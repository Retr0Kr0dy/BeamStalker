#include "sd_card.h"

#ifdef CONFIG_HAS_SDCARD
static const char *SDCardTAG = "SDCard";

void initSDCard() {
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        #ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
        #else
        .format_if_mount_failed = false,
        #endif 
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    const char mount_point[] = MOUNT_POINT;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("Failed to initialize bus.\n");
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    printf("Mounting filesystem\n");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SDCardTAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(SDCardTAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(SDCardTAG, "Filesystem mounted\n");

    sdmmc_card_print_info(stdout, card);

    #ifdef CONFIG_FORMAT_SD_CARD
        ret = esp_vfs_fat_sdcard_format(mount_point, card);
        if (ret != ESP_OK) {
            ESP_LOGE(SDCardTAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
            return;
        }

        if (stat(file_foo, &st) == 0) {
            ESP_LOGI(SDCardTAG, "file still exists");
            return;
        } else {
            ESP_LOGI(SDCardTAG, "file doesn't exist, formatting done");
        }
    #endif
}

void closeSDCard() {
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    if (ret == ESP_OK) {
        ESP_LOGI(SDCardTAG, "Filesystem unmounted successfully.");
    } else {
        ESP_LOGE(SDCardTAG, "Failed to unmount filesystem (%s).", esp_err_to_name(ret));
    }
}

esp_err_t s_write_file(const char *path, const char *data) {
    ESP_LOGI(SDCardTAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(SDCardTAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGI(SDCardTAG, "File written");
    return ESP_OK;
}

esp_err_t s_read_file(const char *path) {
    ESP_LOGI(SDCardTAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(SDCardTAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[256]; 
    fgets(line, sizeof(line), f);
    fclose(f);

    
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(SDCardTAG, "Read from file: '%s'", line);
    return ESP_OK;
}

esp_err_t s_list_dir(const char *path) {
    struct dirent *entry;
    DIR *dp = opendir(path);
    
    if (dp == NULL) {
        ESP_LOGE(SDCardTAG, "Failed to open directory: %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(SDCardTAG, "Listing directory: %s", path);
    while ((entry = readdir(dp))) {
        ESP_LOGI(SDCardTAG, "Found: %s", entry->d_name);
    }
    closedir(dp);
    return ESP_OK;
}

esp_err_t s_create_file(char *path) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        printf("Failed to create file %s, error: %s\n", path, strerror(errno));
        return ESP_FAIL;
    }

    if (fputc('\0', f) == EOF) {
        printf("Failed to write to the file %s, error: %s\n", path, strerror(errno));
        fclose(f);
        return ESP_FAIL;
    }

    fclose(f);

    printf("File %s created successfully\n", path);
    return ESP_OK;
}

esp_err_t s_create_dir(const char *path) {
    char temp_path[512];
    strncpy(temp_path, path, sizeof(temp_path));
    temp_path[sizeof(temp_path) - 1] = '\0';

    char *p = temp_path;

    while ((p = strchr(p, '/')) != NULL) {
        *p = '\0';
        struct stat st;
        if (strlen(temp_path) > 0) {
            if (stat(temp_path, &st) != 0) {
                if (mkdir(temp_path, 0755) != 0 && errno != EEXIST) {
                    ESP_LOGE(SDCardTAG, "Failed to create directory %s, error: %s", temp_path, strerror(errno));
                    return ESP_FAIL;
                }
            } else if (!S_ISDIR(st.st_mode)) {
                ESP_LOGE(SDCardTAG, "%s exists but is not a directory", temp_path);
                return ESP_FAIL;
            }
        }
        *p = '/';
        p++;
    }

    struct stat st;
    if (stat(temp_path, &st) != 0) {
        if (mkdir(temp_path, 0755) != 0 && errno != EEXIST) {
            ESP_LOGE(SDCardTAG, "Failed to create directory %s, error: %s", temp_path, strerror(errno));
            return ESP_FAIL;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        ESP_LOGE(SDCardTAG, "%s exists but is not a directory", temp_path);
        return ESP_FAIL;
    }

    ESP_LOGI(SDCardTAG, "Directory %s is ready", path);
    return ESP_OK;
}

esp_err_t s_remove_file(const char *file_path) {
    if (remove(file_path) != 0) {
        ESP_LOGE(SDCardTAG, "Failed to remove file %s, error: %s", file_path, strerror(errno));
        return ESP_FAIL;
    }

    ESP_LOGI(SDCardTAG, "Successfully removed file: %s", file_path);
    return ESP_OK;
}

esp_err_t s_remove_dir(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[512];

    dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(SDCardTAG, "Failed to open directory %s, error: %s", path, strerror(errno));
        return ESP_FAIL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            ESP_LOGE(SDCardTAG, "Failed to stat %s, error: %s", full_path, strerror(errno));
            closedir(dir);
            return ESP_FAIL;
        }

        if (S_ISDIR(st.st_mode)) {
            ESP_LOGI(SDCardTAG, "Recursively removing directory: %s", full_path);
            esp_err_t err = s_remove_dir(full_path);
            if (err != ESP_OK) {
                ESP_LOGE(SDCardTAG, "Failed to remove directory %s", full_path);
                closedir(dir);
                return err;
            }
        } else {
            ESP_LOGI(SDCardTAG, "Removing file: %s", full_path);
            esp_err_t err = s_remove_file(full_path);
            if (err != ESP_OK) {
                ESP_LOGE(SDCardTAG, "Failed to remove file %s", full_path);
                closedir(dir);
                return err;
            }
        }
    }

    closedir(dir);

    ESP_LOGI(SDCardTAG, "Removing directory: %s", path);
    if (rmdir(path) != 0) {
        ESP_LOGE(SDCardTAG, "Failed to remove directory %s, error: %s", path, strerror(errno));
        return ESP_FAIL;
    }

    ESP_LOGI(SDCardTAG, "Successfully removed directory: %s", path);
    return ESP_OK;
}

void trim_path(char *path) {
    size_t len = strlen(path);
    if (len > 0 && path[len - 1] == '/') {
        path[len - 1] = '\0';
    }
}

bool is_directory_empty(const char *path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        ESP_LOGE(SDCardTAG, "Failed to open directory %s, error: %s", path, strerror(errno));
        return false;  
    }

    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            
            
            if (entry->d_type == DT_DIR) {
                
                if (!is_directory_empty(full_path)) {
                    closedir(dir);
                    return false;
                }
            }
        }
    }

    closedir(dir);
    return true;  
}

esp_err_t s_rename_dir(const char *old_path, const char *new_path) {
    
    char old_path_copy[512];
    char new_path_copy[512];
    strncpy(old_path_copy, old_path, sizeof(old_path_copy));
    strncpy(new_path_copy, new_path, sizeof(new_path_copy));

    
    trim_path(old_path_copy);
    trim_path(new_path_copy);

    
    if (!is_directory_empty(old_path_copy)) {
        ESP_LOGE(SDCardTAG, "Cannot rename directory '%s' because it is not empty", old_path_copy);
        return ESP_FAIL;  
    }

    
    if (rename(old_path_copy, new_path_copy) != 0) {
        ESP_LOGE(SDCardTAG, "Failed to rename directory from %s to %s. Error: %s", 
                  old_path_copy, new_path_copy, strerror(errno));
        return ESP_FAIL;
    }

    ESP_LOGI(SDCardTAG, "Directory renamed from %s to %s", old_path_copy, new_path_copy);
    return ESP_OK;
}

void s_read_bytes_from_file(const char *filename, uint8_t *buffer, size_t size) {
    
    if (buffer == NULL) {
        printf("Provided buffer is NULL!\n");
        return;
    }

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Failed to open file: %s\n", filename);
        return;
    }

    size_t bytesRead = fread(buffer, 1, size, file);
    if (bytesRead < size) {
        if (feof(file)) {
            printf("End of file reached before reading %zu bytes\n", size);
        } else if (ferror(file)) {
            printf("Error reading the file!\n");
        }
    }

    fclose(file);
}

esp_err_t s_write_bytes_to_file(char *path, const uint8_t *data, size_t size) {
    ESP_LOGI(SDCardTAG, "Writing bytes to file %s", path);

    char dir_path[512];
    strncpy(dir_path, path, sizeof(dir_path));
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        struct stat st;
        if (stat(dir_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
            ESP_LOGE(SDCardTAG, "Directory %s is not accessible or does not exist", dir_path);
            return ESP_FAIL;
        }
    }

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        ESP_LOGE(SDCardTAG, "Failed to open file %s for writing, error: %s (%d)", path, strerror(errno), errno);
        return ESP_FAIL;
    }

    size_t bytes_written = fwrite(data, 1, size, f);
    if (bytes_written != size) {
        ESP_LOGE(SDCardTAG, "Failed to write specified number of bytes to %s, wrote %zu of %zu bytes", path, bytes_written, size);
        fclose(f);
        return ESP_FAIL;
    }

    if (fclose(f) != 0) {
        ESP_LOGE(SDCardTAG, "Failed to close file %s after writing, error: %s", path, strerror(errno));
        return ESP_FAIL;
    }

    ESP_LOGI(SDCardTAG, "Successfully wrote %zu bytes to file %s", bytes_written, path);
    return ESP_OK;
}

long long get_file_size(const char *filename) {
    struct stat file_stat;
    
    if (stat(filename, &file_stat) == -1) {
        perror("stat");
        return -1;
    }
    
    printf ("%ld\n",file_stat.st_size);

    return file_stat.st_size;
}

char *get_full_path(const char *base, const char *filename) {
    if (base == NULL || filename == NULL) {
        return NULL;
    }
    size_t path_size = strlen(base) + 1 + strlen(filename) + 1; 

    char *full_path = (char *)malloc(path_size);
    if (full_path == NULL) {
        perror("malloc");
        return NULL;
    }
    snprintf(full_path, path_size, "%s/%s", base, filename);

    printf ("%s\n",full_path);

    return full_path;
}
#endif