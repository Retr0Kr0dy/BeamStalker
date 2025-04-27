extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "linenoise/linenoise.h"
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "firmware/helper.h"
#include "firmware/bitmaps.h"

#include "firmware/modules/hardware/cmd_i2c.h"
#include "firmware/modules/hardware/cmd_spi.h"
#include "firmware/modules/hardware/cmd_radio.h"
#include "firmware/modules/system/cmd_system.h"
#include "firmware/modules/system/baatsh/fg_wrap.h"
#include "firmware/modules/system/baatsh/cmd_baatsh.h"
#include "firmware/modules/wifi/cmd_beaconspam.h"
#include "firmware/modules/wifi/cmd_deauth.h"
#include "firmware/modules/wifi/cmd_wifiscan.h"
#include "firmware/modules/wifi/cmd_wifisniff.h"
#include "firmware/modules/ble/cmd_blespam.h"

static const char* token_start(const char* buf) {
    if (!buf) return buf;
    if (strncmp(buf, "sh ", 3) == 0) buf += 3;
    const char* last = buf;
    for (const char* p = buf; *p; ++p) {
        if (*p=='&' || *p=='|' || *p==';' || isspace((unsigned char)*p)) {
            last = p + 1;
        }
    }
    return last;
}

static void completion_cb(const char* buf, linenoiseCompletions* lc) {
    const char* tok = token_start(buf);
    linenoiseCompletions tmp = {0, NULL};
    esp_console_get_completion(tok, &tmp);
    size_t prefix = tok - buf;
    for (size_t i = 0; i < tmp.len; ++i) {
        const char* s = tmp.cvec[i];
        size_t sl = strlen(s);
        char* full = (char*)malloc(prefix + sl + 1);
        memcpy(full, buf, prefix);
        memcpy(full + prefix, s, sl + 1);
        linenoiseAddCompletion(lc, full);
        free(full);
    }
    for (size_t i = 0; i < tmp.len; ++i) free(tmp.cvec[i]);
    free(tmp.cvec);
}

static char* hint_cb(const char* buf, int* color, int* bold) {
    return (char*)esp_console_get_hint(token_start(buf), color, bold);
}

extern "C" void app_main() {
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    printf("BeamStalker %s\n", VERSION);

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

    esp_console_repl_config_t repl_cfg = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_cfg.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;
    esp_console_dev_uart_config_t uart_cfg = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    esp_console_repl_t* repl = nullptr;

    repl_cfg.prompt = "#";
    repl_cfg.max_cmdline_length = CONFIG_CONSOLE_MAX_COMMAND_LINE_LENGTH;

    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_cfg, &repl_cfg, &repl));

    linenoiseSetCompletionCallback(completion_cb);
    linenoiseSetHintsCallback(hint_cb);
    linenoiseHistorySetMaxLen(50);

    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    esp_log_level_set("*", ESP_LOG_WARN);
}