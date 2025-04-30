#include "cmd_beaconspam.h"

#include "esp_console.h"
#include "argtable3/argtable3.h"
#include <string.h>
#include <stdio.h>
#include "../system/baatsh/signal_ctrl.h"
#include "../modules/system/baatsh/fg_wrap.h"

#include "../../utils/wifi.h"

#define MAX_SSIDS 32

void send_beacon(const char *ssid, const uint8_t *mac_addr, uint8_t channel) {
    uint8_t beacon_frame[128] = {0};
    int ssid_len = strlen(ssid);
    int frame_len = 0;

    beacon_frame[0] = 0x80;
    beacon_frame[1] = 0x00;

    beacon_frame[2] = 0xff;
    beacon_frame[3] = 0xff;

    memset(beacon_frame + 4, 0xff, 6);

    memcpy(beacon_frame + 10, mac_addr, 6);

    memcpy(beacon_frame + 16, mac_addr, 6);

    beacon_frame[22] = 0xc0;
    beacon_frame[23] = 0x6c;

    memset(beacon_frame + 24, 0x01, 8);

    beacon_frame[32] = 0x64;
    beacon_frame[33] = 0x00;

    beacon_frame[34] = 0x01;
    beacon_frame[35] = 0x04;

    frame_len = 36;

    beacon_frame[frame_len++] = 0x00;
    beacon_frame[frame_len++] = ssid_len;
    memcpy(beacon_frame + frame_len, ssid, ssid_len);
    frame_len += ssid_len;

    beacon_frame[frame_len++] = 0x01;
    beacon_frame[frame_len++] = 0x08;
    uint8_t supported_rates[] = {0x82, 0x84, 0x8b, 0x96, 0x12, 0x24, 0x48, 0x6c};
    memcpy(beacon_frame + frame_len, supported_rates, sizeof(supported_rates));
    frame_len += sizeof(supported_rates);

    beacon_frame[frame_len++] = 0x03;
    beacon_frame[frame_len++] = 0x01;
    beacon_frame[frame_len++] = channel;

    esp_wifi_80211_tx(WIFI_IF_STA, beacon_frame, frame_len, false);
}

char *generate_ssid(int charset) {
    char *ssid;

    if (charset == 3) {
        const char *base_ssid = "CamionDeLaDGSI";
        int rand_num = rand() % 9998;

        char buffer[10];
        int num_digits = snprintf(buffer, sizeof(buffer), "%d", rand_num);

        size_t new_length = strlen(base_ssid) + 1 + num_digits + 1;
        char *new_ssid = (char *)malloc(new_length);

        snprintf(new_ssid, new_length, "%s-%d", base_ssid, rand_num);
        return new_ssid;
    }

    const char *hiragana[] = {"あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と"};
    const char *katakana[] = {"ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", "サ", "シ", "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト"};
    const char *cyrillic[] = {"А", "Б", "В", "Г", "Д", "Е", "Ё", "Ж", "З", "И", "Й", "К", "Л", "М", "Н", "О", "П", "Р", "С", "Т", "У", "Ф", "Х", "Ц", "Ч", "Ш", "Щ", "Ъ", "Ы", "Ь", "Э", "Ю", "Я"};

    int num_chars = 8;
    ssid = (char *)malloc(num_chars * 3 + 1);

    if (!ssid) return NULL;

    ssid[0] = '\0';

    for (int i = 0; i < num_chars; i++) {
        const char *char_utf8;
        switch (charset) {
            case 0:
                char_utf8 = hiragana[rand() % (sizeof(hiragana) / sizeof(hiragana[0]))];
                break;
            case 1:
                char_utf8 = katakana[rand() % (sizeof(katakana) / sizeof(katakana[0]))];
                break;
            case 2:
            default:
                char_utf8 = cyrillic[rand() % (sizeof(cyrillic) / sizeof(cyrillic[0]))];
                break;
        }
        strcat(ssid, char_utf8);
    }

    return ssid;
}

void trollBeacon(int charset, const char *custom) {
    char *ssid = NULL;
    bool should_free = false;

    if (custom != NULL) {
        ssid = (char *)custom;
    } else {
        ssid = generate_ssid(charset);
        should_free = true;
    }

    if (ssid == NULL) {
        printf("Failed to generate SSID\n");
        return;
    }

    uint8_t mac_addr[6];
    generate_random_mac(mac_addr);

    for (int p = 0; p < 1; p++) {
        send_beacon(ssid, mac_addr, CHANNEL);
    }

//    printf ("[BEACONSPAM] Sending frame: %s\n", ssid);

    if (should_free) {
        free(ssid);
    }
}

static struct {
    struct arg_int *charset;
    struct arg_str *customs;
    struct arg_end *end;
} beaconspam_args;

static int do_beaconspam_cmd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **)&beaconspam_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, beaconspam_args.end, argv[0]);
        return 1;
    }

    int charset = 0;
    if (beaconspam_args.charset->count > 0) {
        charset = beaconspam_args.charset->ival[0];
    }

    char *ssid_list[MAX_SSIDS] = {0};
    int ssid_count = 0;

    if (beaconspam_args.customs->count > 0) {
        char *raw = strdup(beaconspam_args.customs->sval[0]);
        char *token = strtok(raw, ",");
        while (token && ssid_count < MAX_SSIDS) {
            ssid_list[ssid_count++] = strdup(token);
            token = strtok(NULL, ",");
        }
        free(raw);
    }

    start_wifi(WIFI_MODE_STA, true);
    vTaskDelay(pdMS_TO_TICKS(300));

    int current = 0;

    printf ("[BEACONSPAM] Spam started\n");

    while (1) {
        if (app_sig_abort()) {
            puts("\nQuitting...");
            break;
        }

        if (ssid_count > 0) {
            trollBeacon(charset, ssid_list[current]);
            current = (current + 1) % ssid_count;
        } else {
            trollBeacon(charset, NULL);
        }

        packet_count++;
        taskYIELD();
    }

    for (int i = 0; i < ssid_count; i++) {
        free(ssid_list[i]);
    }

    stop_wifi();
    vTaskDelay(pdMS_TO_TICKS(100));

    return 0;
}

void module_beaconspam(void)
{
    beaconspam_args.charset = arg_int0("c", "charset", "<id>", "Charset to generate SSID with");
    beaconspam_args.customs = arg_str0("s", "ssid", "<ssid,...>", "Custom SSID to use");
    beaconspam_args.end = arg_end(2);
    const esp_console_cmd_t beaconspam_cmd = {
        .command = "beaconspam",
        .help = "Spam wifi beacon generated randomly or with custom ssid",
        .hint = NULL,
        .func = &do_beaconspam_cmd,
        .argtable = &beaconspam_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&beaconspam_cmd));
}
