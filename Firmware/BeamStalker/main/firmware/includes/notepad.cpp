#include "notepad.h"

#ifdef CONFIG_HAS_SDCARD

#define PATH_MAX_LENGTH 512


// A helper function to prompt for a filename using the keyboard.
// This simplistic implementation builds a filename string until the user presses "KEY_ENTER".
int promptFilename(char* buffer, size_t bufferSize) {
    buffer[0] = '\0';
    std::string fileName = "";
    bool done = false;
    
    // Display prompt using the provided display macros.
    clearScreen();
    displayText(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10,"Enter filename:");
    
    while (!done) {
        updateBoard();
        if (anyPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            std::string keyStr = "";
            // Build a string from each pressed key (convert char to string).
            for (auto key : status.word) {
                if (!keyStr.empty()) {
                    keyStr = keyStr + "+" + std::string(1, key);
                } else {
                    keyStr += std::string(1, key);
                }
            }
            // If the user presses KEY_ENTER, finish the prompt.
            if (keyStr.find(KEY_ENTER) != std::string::npos) {
                done = true;
                continue;
            }
            // Handle KEY_BACKSPACE.
            if (keyStr.find(KEY_BACKSPACE) != std::string::npos && !fileName.empty()) {
                fileName.pop_back();
            }
            // Otherwise, append the first key pressed.
            else if (!status.word.empty()) {
                std::string newChar(1, status.word[0]);
                if (newChar != "KEY_LEFT_CTRL" && newChar != "KEY_ENTER" && newChar != "KEY_BACKSPACE") {
                    fileName += newChar;
                }
            }
            
            // Update the display with the current filename.
            clearScreen();
            displayText(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 20, "Filename:");
            displayText(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, fileName.c_str());
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    // Copy the resulting filename into the provided buffer.
    strncpy(buffer, fileName.c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    return 0;
}

// The notepad app function.
// If initPath is non-empty, the app will attempt to load that file's content.
// Otherwise, the text buffer starts empty and will prompt for a filename on save.
int APP_Notepad(const char* initPath) {
    char filePath[PATH_MAX_LENGTH];
    filePath[0] = '\0';
    if (initPath && strlen(initPath) > 0) {
        strncpy(filePath, initPath, PATH_MAX_LENGTH - 1);
        filePath[PATH_MAX_LENGTH - 1] = '\0';
    }
    
    // Text buffer for page content.
    std::string textBuffer;
    
    // If a file path was provided, attempt to read its content.
    if (strlen(filePath) > 0) {
        // Replace with your actual file-read function.
        // For demonstration, assume reading failed or file is empty.
        if (s_read_file(filePath) == ESP_OK) {
            // TODO: Load file content into textBuffer.
            textBuffer = "";
        } else {
            textBuffer = "";
        }
    }
    
    // Display initial content.
    clearScreen();
    displayText(0,0,textBuffer.c_str());
    while (1) {
        updateBoard();
        // Check for keyboard events.
        if (anyPressed()) {
            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            std::string keyStr = "";
            for (auto key : status.word) {
                if (!keyStr.empty()) {
                    keyStr = keyStr + "+" + std::string(1, key);
                } else {
                    keyStr += std::string(1, key);
                }
            }
            
            // Check for KEY_LEFT_CTRL+S to save.
            if ((keyStr.find(KEY_LEFT_CTRL) != std::string::npos) && (keyStr.find("S") != std::string::npos)) {
                // If no filename exists, prompt for one.
                if (strlen(filePath) == 0) {
                    promptFilename(filePath, sizeof(filePath));
                }
                // Save the text buffer to file.
                s_write_file(filePath, textBuffer.c_str());
                clearScreen();
                displayText(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, "File saved");
                vTaskDelay(pdMS_TO_TICKS(1000));
                // Redraw the text after saving.
                clearScreen();
                displayText(0, 0, textBuffer.c_str());
            }
            // Check for KEY_LEFT_CTRL+X to exit.
            else if ((keyStr.find(KEY_LEFT_CTRL) != std::string::npos) && (keyStr.find("X") != std::string::npos)) {
                return 0;
            }
            // Otherwise, treat the key press as text input.
            else {
                if (!status.word.empty()) {
                    std::string newKey(1, status.word[0]);
                    // Handle KEY_BACKSPACE: remove the last character.
                    if (newKey.find(KEY_BACKSPACE) != std::string::npos && !textBuffer.empty()) {
                        textBuffer.pop_back();
                    }
                    // Filter out common control keys.
                    else if (newKey != "KEY_LEFT_CTRL" && newKey != "KEY_ENTER" && newKey != "KEY_BACKSPACE") {
                        textBuffer += newKey;
                    }
                }
                // Update displayed text.
                clearScreen();
                displayText(0, 0, textBuffer.c_str());
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    return 0;
}
#endif