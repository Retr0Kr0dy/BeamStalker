#include "file_manager.h"

#ifdef CONFIG_HAS_SDCARD

#define PATH_MAX_LENGTH 512
#define MAX_LENGTH 24

void cropName(const char* src, char* dest) {
    strncpy(dest, src, MAX_LENGTH - 1);
    dest[MAX_LENGTH - 1] = '\0';
}

void copyFullName(const char* src, char* dest, size_t dest_size) {
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

int APP_FileManager() {
    char current_path[PATH_MAX_LENGTH];
    strcpy(current_path, MOUNT_POINT);

    while (1) {
        int count = 1; // "../"
        DIR* dp = opendir(current_path);
        if (dp == NULL) {
            LogError("ERROR: Cannot open directory");
            vTaskDelay(pdMS_TO_TICKS(1000));
            return -1;
        }
        struct dirent* entry;
        while ((entry = readdir(dp)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            count++;
        }
        closedir(dp);

        struct menu Menu;
        Menu.name = current_path;
        Menu.length = count;
        Menu.elements = new item[Menu.length];

        cropName("../", Menu.elements[0].name);
        Menu.elements[0].type = 1;
        Menu.elements[0].length = 0;
        for (int i = 0; i < MAX_OPTIONS; i++) {
            Menu.elements[0].options[i] = NULL;
        }

        dp = opendir(current_path);
        int idx = 1;
        if (dp != NULL) {
            while ((entry = readdir(dp)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                copyFullName(entry->d_name, Menu.elements[idx].name, MAX_LENGTH*sizeof(char));
                Menu.elements[idx].type = 1;
                Menu.elements[idx].length = 0;
                for (int i = 0; i < MAX_OPTIONS; i++) {
                    Menu.elements[idx].options[i] = NULL;
                }
                
                idx++;
            }
            closedir(dp);
        }

        int Selector = 0;
        bool stayInThisMenu = true;

        drawMenu(Menu, Selector);
        while (stayInThisMenu) {
            updateBoard();
            if (anyPressed()) {
                if (returnPressed()) {
                    delete[] Menu.elements;
                    return 0;
                }
                else if (upPressed()) {
                    Selector = intChecker(Selector - 1, Menu.length);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                else if (downPressed()) {
                    Selector = intChecker(Selector + 1, Menu.length);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                else if (selectPressed()) {
                    clearScreen();
                    vTaskDelay(pdMS_TO_TICKS(300));

                    if (Selector == 0) {  // "../" selected
                        if (strcmp(current_path, MOUNT_POINT) != 0) {
                            char* last_slash = strrchr(current_path, '/');
                            if (last_slash != NULL && last_slash != current_path) {
                                *last_slash = '\0';
                            } else {
                                strcpy(current_path, MOUNT_POINT);
                            }
                        }
                        stayInThisMenu = false;
                    }
                    else {
                        char new_path[PATH_MAX_LENGTH];
                        int max_current_len = sizeof(new_path) - strlen(Menu.elements[Selector].name) - 2;
                        snprintf(new_path, sizeof(new_path), "%.*s/%s", max_current_len, current_path, Menu.elements[Selector].name);
                        struct stat st;
                        if (stat(new_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                            strcpy(current_path, new_path);
                            stayInThisMenu = false;
                        } else {
                            displayText(0, 0, "File selected:");
                            displayText(0, 2, Menu.elements[Selector].name);
                            displayText(0, 4, ">> TODO");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            clearScreen();
                        }
                    }
                }
                drawMenu(Menu, Selector);
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        delete[] Menu.elements;
    }
    return 0;
}


#endif