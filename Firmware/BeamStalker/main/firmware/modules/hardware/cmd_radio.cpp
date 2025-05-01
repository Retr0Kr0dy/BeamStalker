#include <cstring>
#include <cstdlib>
#include <RadioLib.h>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_radio_hal.h"

extern "C" {
#include "cmd_spi.h"
}

#define MAX_RADIOS 4

struct RadioSlot {
    bool            used{};
    int             spi_chan{};
    EspRadioHal    *hal{};
    Module         *mod{};
    PhysicalLayer  *iface{};
    char           *name{};
};

static RadioSlot radios[MAX_RADIOS]{};

class CC1101_Hack : public CC1101 {
public:
    using CC1101::SPIreadRegister;
    using CC1101::SPIwriteRegister;
    using CC1101::SPIsendCommand;
    CC1101_Hack(Module *mod) : CC1101(mod) {}
};

#define RADIOLIB_CC1101_CMD_CAL   0x33   // calibrate
#define RADIOLIB_CC1101_CMD_RX    0x34   // go to RX
#define RADIOLIB_CC1101_CMD_TX    0x35   // go to TX
#define RADIOLIB_CC1101_CMD_IDLE  0x36   // go to IDLE
#define RADIOLIB_CC1101_CMD_SFRX  0x3A   // flush RX FIFO

static PhysicalLayer* make_iface(const char *d, Module *m, int16_t &st) {
    if (!strcasecmp(d, "sx126x") || !strcasecmp(d, "sx1262") || !strcasecmp(d, "sx1261")) {
        auto *p = new SX1262(m); st = p->begin(); return p;
    }
    if (!strcasecmp(d, "sx127x") || !strcasecmp(d, "rfm95") || !strcasecmp(d, "rfm96")) {
        auto *p = new SX1278(m); st = p->begin(); return p;
    }
    // if (!strcasecmp(d, "cc1101") || !strcasecmp(d, "cc110x")) {
    //     auto *p = new CC1101(m); st = p->begin(); return p;
    // }
    if (!strcasecmp(d, "cc1101") || !strcasecmp(d, "cc110x")) {
        auto *p = new CC1101_Hack(m); st = p->begin(); return p;
    }
    st = RADIOLIB_ERR_UNKNOWN;
    return nullptr;
}

static const char* radiolib_strerror(int code) {
    switch (code) {
        case RADIOLIB_ERR_NONE: return "OK";
        case RADIOLIB_ERR_INVALID_FREQUENCY: return "INVALID_FREQUENCY";
        case RADIOLIB_ERR_INVALID_BIT_RATE: return "INVALID_BIT_RATE";
        case RADIOLIB_ERR_INVALID_BANDWIDTH: return "INVALID_BANDWIDTH";
        default: return "UNKNOWN_ERROR";
    }
}

static struct {
    struct arg_int  *chan;
    struct arg_str  *drv;
    struct arg_int  *dio1, *rst, *busy;
    struct arg_end  *end;
} init_args;

static struct {
    struct arg_int *slot;
    struct arg_dbl *freq, *bw;
    struct arg_str *mod;
    struct arg_dbl *bitrate;
    struct arg_int *sf, *cr, *pwr;
    struct arg_str *hex;
    struct arg_end *end;
} tx_args;

static int cmd_radio_init(int argc, char **argv) {
    void *argtable[] = { init_args.chan, init_args.drv, init_args.dio1, init_args.rst, init_args.busy, init_args.end };
    if (arg_parse(argc, argv, argtable)) {
        arg_print_errors(stderr, init_args.end, argv[0]);
        return 1;
    }
    int chan = init_args.chan->ival[0];
    const char *drv = init_args.drv->sval[0];
    spi_device_handle_t h = spi_get_handle(chan);
    gpio_num_t nss = spi_get_cs_gpio(chan);
    if (!h || nss == (gpio_num_t)-1) {
        printf("radio-init: invalid SPI channel %d\n", chan);
        return 1;
    }
    int dio1 = init_args.dio1->count ? init_args.dio1->ival[0] : RADIOLIB_NC;
    int rst  = init_args.rst->count  ? init_args.rst->ival[0]  : RADIOLIB_NC;
    int busy = init_args.busy->count ? init_args.busy->ival[0] : RADIOLIB_NC;
    int slot = -1;
    for (int i = 0; i < MAX_RADIOS; i++) {
        if (!radios[i].used) { slot = i; break; }
    }
    if (slot < 0) {
        printf("radio-init: no free slots\n");
        return 1;
    }
    auto *hal = new EspRadioHal(h);
    auto *mod = new Module(hal, nss, dio1, rst, busy);
    int16_t st;
    PhysicalLayer *iface = make_iface(drv, mod, st);
    if (!iface) {
        printf("radio-init: unknown driver %s\n", drv);
        return 1;
    }
    if (st != RADIOLIB_ERR_NONE) {
        printf("radio-init: begin failed (%d) %s\n", st, radiolib_strerror(st));
        return 1;
    }
    radios[slot] = { true, chan, hal, mod, iface, strdup(drv) };
    printf("radio-init: slot %d → %s on SPI %d OK\n", slot, drv, chan);

    if (!strcasecmp(drv, "cc1101")) {
        auto *cc = static_cast<CC1101_Hack*>(iface);
        cc->reset();
        hal->delay(1);
        cc->SPIsendCommand(RADIOLIB_CC1101_CMD_CAL);
        hal->delay(1);
    }

    return 0;
}

static int cmd_radio_tx(int argc, char **argv) {
    void *argtable[] = { tx_args.slot, tx_args.freq, tx_args.bw, tx_args.mod, tx_args.bitrate, tx_args.sf, tx_args.cr, tx_args.pwr, tx_args.hex, tx_args.end };
    if (arg_parse(argc, argv, argtable)) {
        arg_print_errors(stderr, tx_args.end, argv[0]);
        return 1;
    }
    int slot = tx_args.slot->ival[0];
    float freq = tx_args.freq->dval[0] * 1e6;
    float bw   = tx_args.bw->dval[0] * 1e3;
    const char *mod = tx_args.mod->sval[0];
    float bitrate = tx_args.bitrate->dval[0];
    int sf = tx_args.sf->ival[0];
    int cr = tx_args.cr->ival[0];
    int pwr = tx_args.pwr->ival[0];
    const char *hex = tx_args.hex->sval[0];
    size_t len = strlen(hex) / 2;
    uint8_t buf[256];
    for (size_t i = 0; i < len; ++i) sscanf(hex + 2*i, "%2hhx", &buf[i]);

    auto *iface = radios[slot].iface;
    if (strcmp(radios[slot].name, "cc1101") == 0) {
        auto *cc = static_cast<CC1101*>(iface);
        cc->setFrequency(freq);
        cc->setBitRate(bitrate);
        cc->setRxBandwidth(bw);
        cc->setOOK(!strcasecmp(mod, "ook"));
        cc->transmit(buf, len);
    } else if (strstr(radios[slot].name, "sx126")) {
        auto *sx = static_cast<SX126x*>(iface);
        sx->setFrequency(freq);
        sx->setBandwidth(bw);
        sx->setSpreadingFactor(sf);
        sx->setCodingRate(cr);
        sx->setOutputPower(pwr);
        sx->transmit(buf, len);
    } else if (strstr(radios[slot].name, "sx127") || strstr(radios[slot].name, "rfm9")) {
        auto *sx = static_cast<SX1278*>(iface);
        sx->setFrequency(freq);
        sx->setBandwidth(bw);
        sx->setSpreadingFactor(sf);
        sx->setCodingRate(cr);
        sx->setOutputPower(pwr);
        sx->transmit(buf, len);
    }
    return 0;
}

static struct {
    struct arg_int *slot;
    struct arg_dbl *freq, *bw;
    struct arg_str *mod;
    struct arg_dbl *bitrate;
    struct arg_int *sf, *cr, *pwr;
    struct arg_end *end;
} rx_args;

static int cmd_radio_rx(int argc, char **argv) {
    void *argtable[] = { rx_args.slot, rx_args.freq, rx_args.bw, rx_args.mod, rx_args.bitrate, rx_args.sf, rx_args.cr, rx_args.pwr, rx_args.end };
    if (arg_parse(argc, argv, argtable)) {
        arg_print_errors(stderr, rx_args.end, argv[0]);
        return 1;
    }

    int slot = rx_args.slot->ival[0];
    if (slot < 0 || slot >= MAX_RADIOS || !radios[slot].used) {
        puts("Invalid slot");
        return 1;
    }

    float freq = rx_args.freq->dval[0] * 1e6;
    float bw   = rx_args.bw->dval[0] * 1e3;
    const char *mod = rx_args.mod->sval[0];
    float bitrate = rx_args.bitrate->dval[0];
    int sf = rx_args.sf->ival[0];
    int cr = rx_args.cr->ival[0];

    uint8_t buf[256];
    int16_t len = -1;
    auto *iface = radios[slot].iface;
//    auto *hal = radios[slot].hal;

    if (strcmp(radios[slot].name, "cc1101") == 0) {
        auto *cc = static_cast<CC1101*>(iface);
        cc->setFrequency(freq);
        cc->setBitRate(bitrate);
        cc->setRxBandwidth(bw);
        cc->setOOK(!strcasecmp(mod, "ook"));

        printf("Receiving...\n");
        cc->startReceive();  // non-blocking start
        auto *hal = radios[slot].hal;
        unsigned long start = hal->millis();
        const unsigned long timeout = 5000;

        while (hal->millis() - start < timeout) {
            if (cc->available()) {
                len = cc->readData(buf, sizeof(buf));
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }

    } else if (strstr(radios[slot].name, "sx126")) {
        auto *sx = static_cast<SX126x*>(iface);
        sx->setFrequency(freq);
        sx->setBandwidth(bw);
        sx->setSpreadingFactor(sf);
        sx->setCodingRate(cr);
        len = sx->receive(buf, sizeof(buf));

    } else if (strstr(radios[slot].name, "sx127") || strstr(radios[slot].name, "rfm9")) {
        auto *sx = static_cast<SX1278*>(iface);
        sx->setFrequency(freq);
        sx->setBandwidth(bw);
        sx->setSpreadingFactor(sf);
        sx->setCodingRate(cr);
        len = sx->receive(buf, sizeof(buf));
    }

    if (len > 0) {
        printf("RX (%d B): ", len);
        for (int i = 0; i < len; ++i) printf("%02X ", buf[i]);
        puts("");
    } else {
        printf("RX failed or timeout (status: %d = %s)\n", len, radiolib_strerror(len));
    }

    return 0;
}

static struct {
    struct arg_int *slot;
    struct arg_dbl *freq;
    struct arg_int *step;
    struct arg_int *count;
    struct arg_int *dwell;
    struct arg_end *end;
} scan_args;

static int cmd_radio_scan(int argc, char **argv) {
  void *argtable[] = {
    scan_args.slot,
    scan_args.freq,
    scan_args.step,
    scan_args.count,
    scan_args.dwell,
    scan_args.end
  };

  if (arg_parse(argc, argv, argtable)) {
    arg_print_errors(stderr, scan_args.end, argv[0]);
    return 1;
  }

  int slot = scan_args.slot->ival[0];
  if (slot < 0 || slot >= MAX_RADIOS || !radios[slot].used) {
    puts("Invalid or uninitialized slot");
    return 1;
  }

  if (strcmp(radios[slot].name, "cc1101") != 0) {
    puts("Scan only supported for CC1101");
    return 1;
  }

  float startFreqMHz = scan_args.freq->dval[0];    // MHz
  int   stepKHz      = scan_args.step->ival[0];    // kHz
  int   count        = scan_args.count->ival[0];
  int   dwellMs      = scan_args.dwell->ival[0];   // ms

  auto *cc  = static_cast<CC1101_Hack*>(radios[slot].iface);
  auto *hal = radios[slot].hal;

  printf("Scanning %d channels from %.3f MHz in %d kHz steps, %d ms dwell...\n",
         count, startFreqMHz, stepKHz, dwellMs);

  for (int i = 0; i < count; i++) {
    float freqMHz = startFreqMHz + (stepKHz * i) / 1000.0f;

    cc->SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);
    cc->SPIsendCommand(RADIOLIB_CC1101_CMD_SFRX);
    cc->setFrequency(freqMHz * 1e6);
    cc->SPIsendCommand(RADIOLIB_CC1101_CMD_RX);

    hal->delay(dwellMs);

    int16_t rssi = cc->getRSSI();
    printf("  %.3f MHz → RSSI = %d dBm\n", freqMHz, rssi);
  }

  cc->SPIsendCommand(RADIOLIB_CC1101_CMD_IDLE);
  return 0;
}

extern "C" void register_radio(void) {
    init_args.chan = arg_int1("c", "chan", "<spi>", "SPI channel");
    init_args.drv  = arg_str1("d", "drv", "<driver>", "Driver: sx126x|sx127x|cc1101");
    init_args.dio1 = arg_int0(NULL,  "dio1", "<gpio>", "IRQ GPIO");
    init_args.rst  = arg_int0(NULL,  "rst",  "<gpio>", "Reset GPIO");
    init_args.busy = arg_int0(NULL,  "busy", "<gpio>", "Busy GPIO");
    init_args.end  = arg_end(5);

    esp_console_cmd_t cmd_init = {
        .command = "radio-init",
        .help = "Initialize radio",
        .hint = NULL,
        .func = &cmd_radio_init,
        .argtable = &init_args,
        .func_w_context = NULL,
        .context = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_init));

    tx_args.slot = arg_int1(NULL, "slot", "<slot>", "Radio slot");
    tx_args.freq = arg_dbl1(NULL, "freq", "<MHz>", "Frequency");
    tx_args.bw   = arg_dbl1(NULL, "bw",   "<kHz>", "Bandwidth");
    tx_args.mod  = arg_str1(NULL, "mod",  "<mod>",  "Modulation: fsk|ook|lora");
    tx_args.bitrate = arg_dbl1(NULL, "bitrate", "<kbps>", "Bit rate (for FSK/OOK)");
    tx_args.sf   = arg_int1(NULL, "sf", "<sf>", "Spreading factor (LoRa)");
    tx_args.cr   = arg_int1(NULL, "cr", "<cr>", "Coding rate (LoRa)");
    tx_args.pwr  = arg_int1(NULL, "pwr", "<dBm>", "Output power");
    tx_args.hex  = arg_str1(NULL, "hex", "<hex>", "Payload hex");
    tx_args.end  = arg_end(9);

    esp_console_cmd_t cmd_tx = {
        .command = "radio-tx",
        .help = "Generic radio transmit",
        .hint = NULL,
        .func = &cmd_radio_tx,
        .argtable = &tx_args,
        .func_w_context = NULL,
        .context = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_tx));

    rx_args.slot = arg_int1(NULL, "slot", "<slot>", "Radio slot");
    rx_args.freq = arg_dbl1(NULL, "freq", "<MHz>", "Frequency");
    rx_args.bw   = arg_dbl1(NULL, "bw",   "<kHz>", "Bandwidth");
    rx_args.mod  = arg_str1(NULL, "mod",  "<mod>",  "Modulation: fsk|ook|lora");
    rx_args.bitrate = arg_dbl1(NULL, "bitrate", "<kbps>", "Bit rate (for FSK/OOK)");
    rx_args.sf   = arg_int1(NULL, "sf", "<sf>", "Spreading factor (LoRa)");
    rx_args.cr   = arg_int1(NULL, "cr", "<cr>", "Coding rate (LoRa)");
    rx_args.pwr  = arg_int1(NULL, "pwr", "<dBm>", "Output power");
    rx_args.end  = arg_end(8);

    esp_console_cmd_t cmd_rx = {
        .command = "radio-rx",
        .help = "Generic radio receive",
        .hint = NULL,
        .func = &cmd_radio_rx,
        .argtable = &rx_args,
        .func_w_context = NULL,
        .context = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_rx));

    scan_args.slot  = arg_int1(NULL, "slot", "<slot>", "Radio slot");
    scan_args.freq  = arg_dbl1(NULL, "freq", "<MHz>", "Start frequency");
    scan_args.step  = arg_int1(NULL, "step", "<kHz>", "Step size");
    scan_args.count = arg_int1(NULL, "count", "<n>", "Number of steps");
    scan_args.dwell = arg_int1(NULL, "dwell", "<ms>", "Dwell time");
    scan_args.end   = arg_end(5);

    esp_console_cmd_t cmd_scan = {
        .command = "radio-scan",
        .help = "Scan frequencies with CC1101",
        .hint = NULL,
        .func = &cmd_radio_scan,
        .argtable = &scan_args,
        .func_w_context = NULL,
        .context = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_scan));
}