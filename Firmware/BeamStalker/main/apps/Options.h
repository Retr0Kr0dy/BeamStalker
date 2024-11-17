#ifndef OPTIONS_H
#define OPTIONS_H

extern void ssd1306_display_text(SSD1306_t * dev, int page, char * text, int text_len, bool invert);
extern void ssd1306_clear_screen(SSD1306_t * dev, bool invert);
extern void ssd1306_bitmaps(SSD1306_t * dev, int xpos, int ypos, uint8_t * bitmap, int width, int height, bool invert);
extern void drawMenu(char* element[], int selector, int lenght, char *name);
extern int intChecker (int value, int lenght);

extern SSD1306_t dev;

int APP_Options(){
	int Selector = 0;
	char* Elements[] = {"System Info       ", "Settings         ", "Developer        "};
	int Lenght = sizeof(Elements) / sizeof(Elements[0]);
    char *Name = "~/Options";

    int UPp, DOWNp, SELECTp, RETURNp;

    vTaskDelay(pdMS_TO_TICKS(100));

	for (;;) {
        UPp = !gpio_get_level(UP_GPIO);
        DOWNp = !gpio_get_level(DOWN_GPIO);
        SELECTp = !gpio_get_level(SELECT_GPIO);
        RETURNp = !gpio_get_level(RETURN_GPIO);

		drawMenu(Elements, Selector, Lenght, Name);

		if (UPp) {
            Selector = intChecker(Selector - 1,sizeof(Elements) / sizeof(Elements[0]));
            vTaskDelay(pdMS_TO_TICKS(100));
            ssd1306_clear_screen(&dev, false);
        }
        if (DOWNp) {
            Selector = intChecker(Selector + 1,sizeof(Elements) / sizeof(Elements[0]));
            vTaskDelay(pdMS_TO_TICKS(100));
            ssd1306_clear_screen(&dev, false);
        }
        if (SELECTp) {
            ssd1306_clear_screen(&dev, false);

            switch (Selector) {
                case 0:
                    ssd1306_display_text(&dev, 0, "Current Firmware:", 17, false);
                    ssd1306_display_text(&dev, 2, VERSION, strlen(VERSION), false);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    while ((gpio_get_level(UP_GPIO)&&gpio_get_level(DOWN_GPIO)&&gpio_get_level(SELECT_GPIO)&&gpio_get_level(RETURN_GPIO))) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    ssd1306_clear_screen(&dev, false);
                    break;
                case 1:
                    ssd1306_display_text(&dev, 0, "Nothing", 7, false);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    while ((gpio_get_level(UP_GPIO)&&gpio_get_level(DOWN_GPIO)&&gpio_get_level(SELECT_GPIO)&&gpio_get_level(RETURN_GPIO))) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    ssd1306_clear_screen(&dev, false);
                    break;
                case 2:
                    ssd1306_display_text(&dev, 2, "Good joob,", 10, false);
                    ssd1306_display_text(&dev, 4, "you develop", 12, false);
                    vTaskDelay(pdMS_TO_TICKS(50));
                    while ((gpio_get_level(UP_GPIO)&&gpio_get_level(DOWN_GPIO)&&gpio_get_level(SELECT_GPIO)&&gpio_get_level(RETURN_GPIO))) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    ssd1306_clear_screen(&dev, false);
                    break;
            }
            ssd1306_clear_screen(&dev, false);
        }

        vTaskDelay(pdMS_TO_TICKS(100));

        if (RETURNp) {
            return 0;
        }
	}
}
#endif
