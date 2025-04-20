#include "wifi_main.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"

static int do_wifiscan_cmd(int argc, char **argv) {
    start_wifi(WIFI_MODE_STA, true);
    vTaskDelay(pdMS_TO_TICKS(100));

    printf ("[WIFISCAN] Scanning...\n"); 

    int ap_count;
    AP* ap_info_list = scan_wifi_ap(&ap_count);

    if (!ap_info_list) {
        printf("No access points found.\n");
        return 1;
    }

    stop_wifi();
    vTaskDelay(pdMS_TO_TICKS(100));

    printf ("[WIFISCAN] Found %d APs :\n", ap_count); 
    for (int i = 0; i < ap_count; i++) {
        printf ("\t%02X:%02X:%02X:%02X:%02X:%02X\t%s\n",
        ap_info_list[i].address[0], ap_info_list[i].address[1], ap_info_list[i].address[2], 
        ap_info_list[i].address[3], ap_info_list[i].address[4], ap_info_list[i].address[5],
        ap_info_list[i].name);
    }

    return 0;
}

void module_wifiscan(void)
{
    const esp_console_cmd_t wifiscan_cmd = {
        .command = "wifiscan",
        .help = "Scan wifi AP available.",
        .hint = NULL,
        .func = &do_wifiscan_cmd,
        .argtable = NULL

    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifiscan_cmd));
}