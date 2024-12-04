#ifndef OPTIONS_H
#define OPTIONS_H

#include <M5Cardputer.h>

extern void M5GFX_display_text(int x, int y, const char* text, uint32_t color = TFT_WHITE);
extern void M5GFX_clear_screen(uint32_t color = TFT_BLACK);
extern void drawMenu(const char* element[], int selector, int length, const char* name);
extern int intChecker(int value, int length);

int APP_Options() {
    int Selector = 0;
    const char* Elements[] = {"System Info", "Settings", "Developer"};
    int Length = sizeof(Elements) / sizeof(Elements[0]);
    const char* Name = "~/Options";

    int UPp, DOWNp, SELECTp, RETURNp;

    vTaskDelay(pdMS_TO_TICKS(100));


    while (1) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange()) {
            // Draw the menu
            drawMenu(Elements, Selector, Length, Name);

            // Read button states (Replace with your specific M5 button setup)
            UPp = M5Cardputer.Keyboard.isKeyPressed(';');
            DOWNp = M5Cardputer.Keyboard.isKeyPressed('.');
            SELECTp = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
            RETURNp = M5Cardputer.Keyboard.isKeyPressed('`');

            if (RETURNp) {
                return 0;
            }

            if (UPp) {
                Selector = intChecker(Selector - 1, Length);
                vTaskDelay(pdMS_TO_TICKS(50));
                M5GFX_clear_screen();
            }
            if (DOWNp) {
                Selector = intChecker(Selector + 1, Length);
                vTaskDelay(pdMS_TO_TICKS(50));
                M5GFX_clear_screen();
            }
            if (SELECTp) {
                M5GFX_clear_screen();
                int wait = 1;
                switch (Selector) {
                    case 0:
                        M5GFX_display_text(0, 0, "Current Firmware:");
                        M5GFX_display_text(0, 20, VERSION);  // Assuming `VERSION` is defined elsewhere
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }

                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        M5GFX_clear_screen();
                        break;
                    case 1:
                        M5GFX_display_text(0, 0, "Nothing");
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        M5GFX_clear_screen();
                        break;
                    case 2:
                        M5GFX_display_text(0, 40, "Good job,");
                        M5GFX_display_text(0, 60, "you develop");
                        vTaskDelay(pdMS_TO_TICKS(200));
                        wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                        M5GFX_clear_screen();
                        break;
                }
                M5GFX_clear_screen();
            }

        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void M5GFX_display_text(int x, int y, const char* text, uint32_t color) {
    M5.Display.setCursor(x, y);
    M5.Display.setTextColor(color);
    M5.Display.print(text);
}

void M5GFX_clear_screen(uint32_t color) {
    M5.Display.fillScreen(color);
}
#endif
