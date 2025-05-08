#include <cstring>
#include <cstdlib>
#include <vector>
#include <memory>
#include <RadioLib.h>

#include "argtable3/argtable3.h"
#include "esp_console.h"

extern "C" {
#include "cmd_spi.h"
}

#include "radio/radio_base.h"
#include "radio/radio_errors.h"

static constexpr int MAX_RADIOS = 4;
struct Slot { bool used = false; std::unique_ptr<RadioDriver> drv; };
static Slot slots[MAX_RADIOS];

static bool is_lora_mod(const char* m)     { return strcasecmp(m,"lora")==0; }
static bool is_fsk_or_ook(const char* m)   {
    return strcasecmp(m,"fsk")==0 || strcasecmp(m,"ook")==0;
}

static struct {
    struct arg_int *chan;
    struct arg_str *drv;
    struct arg_int *dio1,*rst,*busy,*dio2;
    struct arg_end *end;
} init_a;

static struct {
    struct arg_int *slot;
    struct arg_dbl *freq,*bw;
    struct arg_str *mod;
    struct arg_dbl *bitrate;
    struct arg_int *sf,*cr,*pwr;
    struct arg_str *hex;
    struct arg_end *end;
} tx_a;

static struct {
    struct arg_int *slot;
    struct arg_dbl *freq,*bw;
    struct arg_str *mod;
    struct arg_dbl *bitrate;
    struct arg_int *sf,*cr;
    struct arg_end *end;
} rx_a;

static struct {
    struct arg_int *slot;
    struct arg_dbl *freq;
    struct arg_int *step,*count,*dwell;
    struct arg_end *end;
} scan_a;

static struct {
    struct arg_int *slot;
    struct arg_end *end;
} close_a;

static int cmd_radio_init(int argc,char** argv)
{
    void* tbl[] = { init_a.chan,init_a.drv,init_a.dio1,
                    init_a.rst,init_a.busy,init_a.dio2,init_a.end };
    if(arg_parse(argc,argv,tbl)) {
        arg_print_errors(stderr,init_a.end,argv[0]); return 1;
    }

    int chan = init_a.chan->ival[0];
    spi_device_handle_t spi = spi_get_handle(chan);
    gpio_num_t cs = spi_get_cs_gpio(chan);
    if(!spi || cs==(gpio_num_t)-1) { puts("radio-init: bad SPI channel"); return 1; }

    int slot = -1;
    for(int i=0;i<MAX_RADIOS;i++) if(!slots[i].used){ slot=i; break; }
    if(slot<0) { puts("radio-init: no free slots"); return 1; }

    auto drv = make_driver(init_a.drv->sval[0], spi, cs,
                           init_a.dio1->count?init_a.dio1->ival[0]:RADIOLIB_NC,
                           init_a.rst ->count?init_a.rst ->ival[0]:RADIOLIB_NC,
                           init_a.busy->count?init_a.busy->ival[0]:RADIOLIB_NC,
                           init_a.dio2->count ? init_a.dio2->ival[0] : RADIOLIB_NC);
    if(!drv){ printf("radio-init: unknown driver %s\n", init_a.drv->sval[0]); return 1;}

    int16_t st = drv->begin();
    if(st!=RADIOLIB_ERR_NONE){
        printf("radio-init: %s (%d)\n", radiolib_strerror(st), st); return 1;
    }

    slots[slot].drv = std::move(drv);  slots[slot].used = true;
    printf("radio-init: slot %d → %s OK\n", slot, slots[slot].drv->name());
    return 0;
}

static int cmd_radio_tx(int argc,char** argv)
{
    void* tbl[] = { tx_a.slot,tx_a.freq,tx_a.bw,tx_a.mod,tx_a.bitrate,
                    tx_a.sf,tx_a.cr,tx_a.pwr,tx_a.hex,tx_a.end };
    if(arg_parse(argc,argv,tbl)) {
        arg_print_errors(stderr,tx_a.end,argv[0]); return 1;
    }

    int slot = tx_a.slot->ival[0];
    if(slot<0||slot>=MAX_RADIOS||!slots[slot].used){
        puts("radio-tx: invalid slot"); return 1;
    }

    const char* mod = tx_a.mod->sval[0];
    if(is_lora_mod(mod) && (!tx_a.sf->count || !tx_a.cr->count)){
        puts("radio-tx: --sf and --cr required for LoRa"); return 1;
    }
    if(is_fsk_or_ook(mod) && (!tx_a.bitrate->count || !tx_a.bw->count)){
        puts("radio-tx: --bitrate and --bw required for FSK/OOK"); return 1;
    }

    RadioConfig cfg{};
    cfg.frequency  = tx_a.freq->dval[0];
    if(tx_a.bw->count)      cfg.bandwidth = tx_a.bw->dval[0];
    cfg.modulation = mod;
    if(tx_a.bitrate->count) cfg.bitrate   = tx_a.bitrate->dval[0];
    if(tx_a.sf->count)      cfg.sf        = tx_a.sf->ival[0];
    if(tx_a.cr->count)      cfg.cr        = tx_a.cr->ival[0];
    cfg.power      = tx_a.pwr->ival[0];

    std::vector<uint8_t> payload; size_t n=strlen(tx_a.hex->sval[0]);
    if(n%2){ puts("radio-tx: odd hex length"); return 1; }
    payload.resize(n/2);
    for(size_t i=0;i<payload.size();++i)
        sscanf(tx_a.hex->sval[0]+2*i,"%2hhx",&payload[i]);

    int16_t st = slots[slot].drv->tx(cfg,payload.data(),payload.size());
    printf("radio-tx: %s (%d)\n", radiolib_strerror(st), st);
    return st==RADIOLIB_ERR_NONE?0:1;
}

static int cmd_radio_rx(int argc,char** argv)
{
    void* tbl[] = { rx_a.slot,rx_a.freq,rx_a.bw,rx_a.mod,rx_a.bitrate,
                    rx_a.sf,rx_a.cr,rx_a.end };
    if(arg_parse(argc,argv,tbl)){
        arg_print_errors(stderr,rx_a.end,argv[0]); return 1;
    }

    int slot = rx_a.slot->ival[0];
    if(slot<0||slot>=MAX_RADIOS||!slots[slot].used){
        puts("radio-rx: invalid slot"); return 1;
    }

    const char* mod = rx_a.mod->sval[0];
    if(is_lora_mod(mod) && (!rx_a.sf->count || !rx_a.cr->count)){
        puts("radio-rx: --sf and --cr required for LoRa"); return 1;
    }
    if(is_fsk_or_ook(mod) && !rx_a.bitrate->count){
        puts("radio-rx: --bitrate required for FSK/OOK"); return 1;
    }

    RadioConfig cfg{};
    cfg.frequency  = rx_a.freq->dval[0];
    if(rx_a.bw->count)      cfg.bandwidth = rx_a.bw->dval[0];
    cfg.modulation = mod;
    if(rx_a.bitrate->count) cfg.bitrate   = rx_a.bitrate->dval[0];
    if(rx_a.sf->count)      cfg.sf        = rx_a.sf->ival[0];
    if(rx_a.cr->count)      cfg.cr        = rx_a.cr->ival[0];

    std::vector<uint8_t> buf;
    int16_t st = slots[slot].drv->rx(cfg,buf,5000);

    if(st>0){
        printf("radio-rx: %d B → ", st);
        for(uint8_t b:buf) printf("%02X ", b);
        puts(""); return 0;
    }
    printf("radio-rx: %s (%d)\n", radiolib_strerror(st), st);
    return 1;
}

static int cmd_radio_scan(int argc,char** argv)
{
    void* tbl[] = { scan_a.slot,scan_a.freq,scan_a.step,
                    scan_a.count,scan_a.dwell,scan_a.end };
    if(arg_parse(argc,argv,tbl)){
        arg_print_errors(stderr,scan_a.end,argv[0]); return 1;
    }

    int slot = scan_a.slot->ival[0];
    if(slot<0||slot>=MAX_RADIOS||!slots[slot].used){
        puts("radio-scan: invalid slot"); return 1;
    }

    float start_Hz = scan_a.freq->dval[0];
    int   step_kHz = scan_a.step ->ival[0];
    int   count    = scan_a.count->ival[0];
    int   dwell_ms = scan_a.dwell->ival[0];

    int16_t st = slots[slot].drv->scan(start_Hz,step_kHz,count,dwell_ms);
    if(st==RADIOLIB_ERR_UNSUPPORTED){
        puts("radio-scan: driver does not support scanning"); return 1;
    }
    return st==RADIOLIB_ERR_NONE?0:1;
}

static int cmd_radio_close(int argc,char** argv)
{
    void* tbl[] = { close_a.slot, close_a.end };
    if(arg_parse(argc,argv,tbl)){
        arg_print_errors(stderr,close_a.end,argv[0]); return 1;
    }

    int slot = close_a.slot->ival[0];
    if(slot<0||slot>=MAX_RADIOS||!slots[slot].used){
        puts("radio-close: invalid slot"); return 1;
    }

    slots[slot].drv.reset();
    slots[slot].used = false;
    printf("radio-close: slot %d released\n", slot);
    return 0;
}
extern "C" void register_radio(void)
{
    init_a.chan = arg_int1("c","chan","<spi>","SPI bus index (0,1,2 …)");
    init_a.drv  = arg_str1("d","drv","<driver>","Radio type: cc1101 | sx126x | sx127x");
    init_a.dio1 = arg_int0(NULL,"dio1","<gpio>","IRQ (GDO0)    default=NC");
    init_a.rst  = arg_int0(NULL,"rst","<gpio>","RESET         default=NC");
    init_a.busy = arg_int0(NULL,"busy","<gpio>","BUSY (SX126x) default=NC");
    init_a.dio2 = arg_int0(NULL,"dio2","<gpio>","GDO2 (TX done, CC1101) default=NC");
    init_a.end  = arg_end(7);

    esp_console_cmd_t cmd_init{};
    cmd_init.command  = "radio-init";
    cmd_init.hint     = "-c <spi> -d <driver> [extra GPIO flags]";
    cmd_init.help =
        "Initialise one radio, bind it to a free slot and power it up.\n"
        "\n"
        "Minimal examples\n"
        "  • CC1101 on SPI0, CSn=5,   GDO0=2, GDO2=23, RST=15\n"
        "      radio-init -c 0 -d cc1101 --dio1 2 --dio2 23 --rst 15\n"
        "  • SX1262 on SPI1, CS=17,   BUSY=35, DIO1=36, RST=34\n"
        "      radio-init -c 1 -d sx126x --busy 35 --dio1 36 --rst 34\n"
        "\n"
        "Returns: slot index to be used by other commands.";
    cmd_init.func     = &cmd_radio_init;
    cmd_init.argtable = &init_a;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_init));

    tx_a.slot    = arg_int1(NULL,"slot","<n>","Slot from radio‑init");
    tx_a.freq    = arg_dbl1(NULL,"freq","<MHz>","Carrier frequency");
    tx_a.bw      = arg_dbl0(NULL,"bw","<kHz>","RX BW (CC1101: 58‑812) default=auto");
    tx_a.mod     = arg_str1(NULL,"mod","<m>","fsk | ook | lora");
    tx_a.bitrate = arg_dbl0(NULL,"bitrate","<kbps>","FSK/OOK only");
    tx_a.sf      = arg_int0(NULL,"sf","<6‑12>","LoRa spreading factor");
    tx_a.cr      = arg_int0(NULL,"cr","<5‑8>","LoRa coding rate");
    tx_a.pwr     = arg_int1(NULL,"pwr","<dBm>","TX power (driver limits)");
    tx_a.hex     = arg_str1(NULL,"hex","<hex>","Payload as hex (no spaces)");
    tx_a.end     = arg_end(9);

    esp_console_cmd_t cmd_tx{};
    cmd_tx.command  = "radio-tx";
    cmd_tx.hint     = "--slot <n> --freq <MHz> --mod <m> --pwr <dBm> --hex <data>";
    cmd_tx.help =
        "Transmit one frame. The command blocks until the packet is on air,\n"
        "or a timeout/error occurs.\n"
        "\n"
        "Typical CC1101 example (38.4 kbps FSK)\n"
        "  radio-tx --slot 0 --freq 433.92 --mod fsk --bitrate 38.4 \\\n"
        "           --bw 101.6 --pwr 10 --hex 48656C6C6F\n"
        "\n"
        "Typical LoRa example (EU868 beacons)\n"
        "  radio-tx --slot 1 --freq 868.1 --mod lora --sf 7 --cr 5 \\\n"
        "           --bw 125 --pwr 14 --hex DEADBEEF";
    cmd_tx.func     = &cmd_radio_tx;
    cmd_tx.argtable = &tx_a;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_tx));

    rx_a.slot    = arg_int1(NULL,"slot","<n>","Slot");
    rx_a.freq    = arg_dbl1(NULL,"freq","<MHz>","Frequency");
    rx_a.bw      = arg_dbl0(NULL,"bw","<kHz>","Bandwidth (kHz)");
    rx_a.mod     = arg_str1(NULL,"mod","<m>","fsk | ook | lora");
    rx_a.bitrate = arg_dbl0(NULL,"bitrate","<kbps>","FSK/OOK only");
    rx_a.sf      = arg_int0(NULL,"sf","<sf>","LoRa only");
    rx_a.cr      = arg_int0(NULL,"cr","<cr>","LoRa only");
    rx_a.end     = arg_end(7);

    esp_console_cmd_t cmd_rx{};
    cmd_rx.command  = "radio-rx";
    cmd_rx.hint     = "--slot <n> --freq <MHz> --mod <m> [extra flags]";
    cmd_rx.help =
        "Receive a single frame (5 s timeout). Prints RSSI and payload.\n"
        "\n"
        "Example: sniff 433.92 MHz OOK doorbell bursts @ 2.4 kbps\n"
        "  radio-rx --slot 0 --freq 433.92 --mod ook --bitrate 2.4 --bw 67.7";
    cmd_rx.func     = &cmd_radio_rx;
    cmd_rx.argtable = &rx_a;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_rx));

    scan_a.slot  = arg_int1(NULL,"slot","<n>","Slot");
    scan_a.freq  = arg_dbl1(NULL,"freq","<MHz>","Start frequency");
    scan_a.step  = arg_int1(NULL,"step","<kHz>","Step size");
    scan_a.count = arg_int1(NULL,"count","<n>","Number of channels");
    scan_a.dwell = arg_int1(NULL,"dwell","<ms>","Dwell per channel");
    scan_a.end   = arg_end(5);

    esp_console_cmd_t cmd_scan{};
    cmd_scan.command  = "radio-scan";
    cmd_scan.hint     = "--slot <n> --freq <MHz> --step <kHz> --count <n>";
    cmd_scan.help =
        "Sweeps RSSI over a frequency range. Driver must support scan().\n"
        "\n"
        "Example: 433 MHz ISM band with 25 kHz resolution\n"
        "  radio-scan --slot 0 --freq 433 --step 25 --count 40 --dwell 10";
    cmd_scan.func     = &cmd_radio_scan;
    cmd_scan.argtable = &scan_a;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_scan));

    close_a.slot = arg_int1(NULL,"slot","<n>","Slot");
    close_a.end  = arg_end(1);

    esp_console_cmd_t cmd_close{};
    cmd_close.command  = "radio-close";
    cmd_close.hint     = "--slot <n>";
    cmd_close.help =
        "Powers the radio down and frees the slot for reuse.";
    cmd_close.func     = &cmd_radio_close;
    cmd_close.argtable = &close_a;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_close));
}
