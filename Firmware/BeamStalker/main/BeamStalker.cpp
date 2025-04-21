extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#include "esp_console.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "cmd_system.h"

#include "firmware/helper.h"
#include "firmware/bitmaps.h"
#include "firmware/interface.h"

#include "firmware/apps/Wifi/beaconspam_cmd.h"
#include "firmware/apps/Wifi/deauth_cmd.h"
#include "firmware/apps/Wifi/wifiscan_cmd.h"
#include "firmware/apps/Wifi/wifisniff_cmd.h"
#include "firmware/apps/BLE/blespam_cmd.h"

extern "C" void app_main(void) {
    initBoard();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("BeamStalker %s\n", VERSION);

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    repl_config.prompt = "striker:>";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    /* Loads Modules */
    esp_console_register_help_command();

    register_system();
    module_beaconspam();
    module_deauth();
    module_wifiscan();
    module_wifisniff();
    module_blespam();

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#else
#error Unsupported console type
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    /* Prod ready no log after startup*/
    esp_log_level_set("*", ESP_LOG_NONE);
}
