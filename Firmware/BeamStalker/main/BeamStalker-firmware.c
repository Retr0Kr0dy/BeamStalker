#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "time.h"

#include "ssd1306.h"

#include "helper.h"
#include "bitmaps.h"

#include "apps/Options.h"
#include "apps/wifcker.h"

SSD1306_t dev;

/*

  Functions

*/

int intChecker (int value, int lenght) {
	while (value < 0) {
		value += lenght;
	}
	while (value > lenght-1) {
		value -= lenght;
	}

	return value;
}

void drawMenu(char* element[], int selector, int lenght, char *menuName) {
//	int i1 = intChecker(selector-2,lenght);
	int i2 = intChecker(selector-1,lenght);
	int i4 = intChecker(selector+1,lenght);

//    ssd1306_display_text(&dev, 0, element[i1], strlen(element[i1]), false);
    ssd1306_display_text(&dev, 0, menuName, strlen(menuName), false);
    ssd1306_display_text(&dev, 2, element[i2], strlen(element[i2]), false);
    ssd1306_display_text(&dev, 4, element[selector], strlen(element[selector]), true);
    ssd1306_display_text(&dev, 6, element[i4], strlen(element[i4]), false);
}



void mainTask(void *pvParameters) {
    ssd1306_clear_screen(&dev, false);
    vTaskDelay(pdMS_TO_TICKS(100));

    int MainMenuSelector = 0;
	char* MainMenuElements[] = {"WiFcker          ", "The Eye           ", "Options         "};
	int MainMenuLenght = sizeof(MainMenuElements) / sizeof(MainMenuElements[0]);
    char *MainMenuName = "~/";

    for (;;) {
        int UPp, DOWNp, SELECTp;
//, RETURNp;

        while(1) {
        	drawMenu(MainMenuElements, MainMenuSelector, MainMenuLenght, MainMenuName);

            UPp = !gpio_get_level(UP_GPIO);
            DOWNp = !gpio_get_level(DOWN_GPIO);
            SELECTp = !gpio_get_level(SELECT_GPIO);
//            RETURNp = gpio_get_level(RETURN_GPIO);

        	if (UPp) {
                MainMenuSelector = intChecker(MainMenuSelector - 1,sizeof(MainMenuElements) / sizeof(MainMenuElements[0]));
                vTaskDelay(pdMS_TO_TICKS(50));
                ssd1306_clear_screen(&dev, false);
        	}
        	else if (DOWNp) {
        		MainMenuSelector = intChecker(MainMenuSelector + 1,sizeof(MainMenuElements) / sizeof(MainMenuElements[0]));
                vTaskDelay(pdMS_TO_TICKS(50));
                ssd1306_clear_screen(&dev, false);
        	}
        	else if (SELECTp) {
                vTaskDelay(pdMS_TO_TICKS(50));
                switch (MainMenuSelector) {
                    int ret;
                    case 2:
                        ssd1306_clear_screen(&dev, false);
            			ret = APP_Options();
                        if (ret!=0) {printf ("Error in app.");}
                        break;
                    case 0:
                        ssd1306_clear_screen(&dev, false);
                        ret = APP_WiFcker();
                        if (ret!=0) {printf ("Error in app.");}
                        break;
                }
                ssd1306_clear_screen(&dev, false);
            }
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }
}


void app_main(void) {
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
	ssd1306_init(&dev, 128, 64);

    ssd1306_contrast(&dev, 0xff);
    ssd1306_clear_screen(&dev, false);

	ssd1306_bitmaps(&dev, 0, 0, skullLogo, 128, 64, false);
    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(UP_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(DOWN_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(SELECT_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(RETURN_GPIO, GPIO_MODE_INPUT);

    gpio_set_pull_mode(UP_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(DOWN_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(SELECT_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(RETURN_GPIO, GPIO_PULLUP_ONLY);

    char *name = "BeamStalker";

    ssd1306_display_text(&dev, 4, name, strlen(name), false);
    ssd1306_display_text(&dev, 5, VERSION, strlen(VERSION), false);

    printf ("%s %s\n",name,VERSION);

    vTaskDelay(pdMS_TO_TICKS(500));
    ssd1306_display_text(&dev, 7, "Press to boot...", 16, false);

    while ((gpio_get_level(BUTTON_GPIO)&&gpio_get_level(UP_GPIO)&&gpio_get_level(DOWN_GPIO)&&gpio_get_level(SELECT_GPIO)&&gpio_get_level(RETURN_GPIO))) {
        vTaskDelay(pdMS_TO_TICKS(30));
    }

    ssd1306_clear_screen(&dev, false);
    xTaskCreate(mainTask, "BeamStalker", 4096, NULL, 1, NULL);
}
