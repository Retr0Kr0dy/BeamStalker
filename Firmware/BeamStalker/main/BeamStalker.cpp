extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
}

#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <cctype>
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_dev.h"

#include "firmware/modules/hardware/cmd_i2c.h"
#include "firmware/modules/hardware/cmd_spi.h"
#include "firmware/modules/hardware/cmd_radio.h"

#include "firmware/helper.h"
#include "firmware/bitmaps.h"

#include "firmware/modules/system/cmd_system.h"
#include "firmware/modules/system/baatsh/fg_wrap.h"
#include "firmware/modules/system/baatsh/cmd_baatsh.h"
#include "firmware/modules/wifi/cmd_beaconspam.h"
#include "firmware/modules/wifi/cmd_deauth.h"
#include "firmware/modules/wifi/cmd_wifiscan.h"
#include "firmware/modules/wifi/cmd_wifisniff.h"
#include "firmware/modules/ble/cmd_blespam.h"

static const char *token_start(const char *buf)
{
    if (strncmp(buf, "sh ", 3) == 0) buf += 3;
    const char *last = buf;
    for (const char *p = buf; *p; ++p) {
        if (*p=='&'||*p=='|'||*p==';'||isspace((unsigned char)*p))
            last = p + 1;
    }
    return last;
}

static void completion_cb(const char *buf, linenoiseCompletions *lc)
{
    const char *tok = token_start(buf);
    size_t prefix_len = (size_t)(tok - buf);

    linenoiseCompletions tmp = { 0, nullptr };
    esp_console_get_completion(tok, &tmp);

    for (size_t i = 0; i < tmp.len; ++i) {
        const char *suggest = tmp.cvec[i];
        size_t newlen = prefix_len + strlen(suggest) + 1;
        char *full = (char *)malloc(newlen);
        memcpy(full, buf, prefix_len);
        strcpy(full + prefix_len, suggest);
        linenoiseAddCompletion(lc, full);
        free(full);
    }

    for (size_t i = 0; i < tmp.len; ++i) free(tmp.cvec[i]);
    free(tmp.cvec);
}

static char *hint_cb(const char *buf, int *color, int *bold)
{
    return (char *)esp_console_get_hint(token_start(buf), color, bold);
}

extern "C" void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("BeamStalker %s\n", VERSION);

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "striker:>";
    repl_config.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    esp_console_register_help_command();
    register_system();
    register_builtins();

    register_i2c();
    register_spi();
    register_radio();

    module_beaconspam();
    module_deauth();
    module_wifiscan();
    module_wifisniff();
    module_blespam();

#if CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    esp_console_dev_usb_serial_jtag_config_t usb_cfg =
        ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(
        esp_console_new_repl_usb_serial_jtag(&usb_cfg, &repl_config, &repl)
    );
#elif CONFIG_ESP_CONSOLE_UART_DEFAULT || CONFIG_ESP_CONSOLE_UART_CUSTOM
    esp_console_dev_uart_config_t uart_cfg =
        ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(
        esp_console_new_repl_uart(&uart_cfg, &repl_config, &repl)
    );
#else
    #error "Unsupported console type"
#endif

    setvbuf(stdout, NULL, _IONBF, 0);

    linenoiseSetMultiLine(0);
    linenoiseSetCompletionCallback(completion_cb);
    linenoiseSetHintsCallback(hint_cb);
    linenoiseHistorySetMaxLen(50);

    signal_ctrl_init();

    xTaskCreatePinnedToCore([](void *){
            const char *p =  
    "\033[0;32m# \033[0m";
            for (;;) {
                fflush(stdout);
                char *line = linenoise(p);
                if (!line) continue;
                if (*line) {
                    linenoiseHistoryAdd(line);
                    shell_run_fg((const char *)line);
                }
                linenoiseFree(line);
            }
        }, "repl", 4096, NULL, tskIDLE_PRIORITY + 5, NULL, tskNO_AFFINITY);

    esp_log_level_set("*", ESP_LOG_WARN);
}