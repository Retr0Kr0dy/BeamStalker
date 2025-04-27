#include "cmd_spi.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SPI_LINES     2
#define MAX_SPI_CHANNELS 16

typedef struct {
    bool              in_use;
    gpio_num_t        miso_io, mosi_io, sck_io;
    spi_host_device_t host;
} spi_line_t;

typedef struct {
    bool                in_use;
    uint8_t             line_id;
    gpio_num_t          cs_io;
    spi_device_handle_t handle;
} spi_channel_t;

static spi_line_t    spi_lines[MAX_SPI_LINES]     = {0};
static spi_channel_t spi_chans[MAX_SPI_CHANNELS] = {0};

static struct { struct arg_int *miso, *mosi, *sck; struct arg_end *end; } add_line_args;
static struct { struct arg_int *line, *cs, *clk;  struct arg_end *end; } add_chan_args;
static struct { struct arg_int *chan; struct arg_str *data; struct arg_end *end; } xfer_args;
static struct { struct arg_int *pin;  struct arg_int *dur; struct arg_end *end; } reset_args;

static int cmd_spi_line_add(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**)&add_line_args)) {
        arg_print_errors(stderr, add_line_args.end, argv[0]);
        return 1;
    }
    for (int i = 0; i < MAX_SPI_LINES; i++) {
        if (!spi_lines[i].in_use) {
            spi_bus_config_t cfg = {
                .miso_io_num = add_line_args.miso->ival[0],
                .mosi_io_num = add_line_args.mosi->ival[0],
                .sclk_io_num = add_line_args.sck->ival[0],
                .quadwp_io_num = -1,
                .quadhd_io_num = -1,
                .max_transfer_sz = 4096
            };
            spi_host_device_t host = (spi_host_device_t)(SPI2_HOST + i);
            if (spi_bus_initialize(host, &cfg, SPI_DMA_CH_AUTO) != ESP_OK) {
                printf("spi-line-add: bus init failed\n");
                return 1;
            }
            spi_lines[i] = (spi_line_t){ .in_use=true, .miso_io=cfg.miso_io_num,
                                         .mosi_io=cfg.mosi_io_num, .sck_io=cfg.sclk_io_num,
                                         .host=host };
            printf("spi-line-add %d...OK\n", i);
            return 0;
        }
    }
    printf("spi-line-add: no free lines\n");
    return 1;
}

static int cmd_spi_line_list(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("SPI Lines:\n");
    for (int i = 0; i < MAX_SPI_LINES; i++) {
        if (spi_lines[i].in_use) {
            printf("  [%d] MISO=%d MOSI=%d SCK=%d host=%d\n",
                   i,
                   spi_lines[i].miso_io,
                   spi_lines[i].mosi_io,
                   spi_lines[i].sck_io,
                   spi_lines[i].host);
        }
    }
    return 0;
}

static int cmd_spi_channel_add(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**)&add_chan_args)) {
        arg_print_errors(stderr, add_chan_args.end, argv[0]);
        return 1;
    }
    int line = add_chan_args.line->ival[0];
    if (line < 0 || line >= MAX_SPI_LINES || !spi_lines[line].in_use) {
        printf("spi-channel-add: no such line %d\n", line);
        return 1;
    }
    for (int i = 0; i < MAX_SPI_CHANNELS; i++) {
        if (!spi_chans[i].in_use) {
            spi_device_interface_config_t devcfg = {
                .clock_speed_hz = add_chan_args.clk->ival[0],
                .mode = 0,
                .spics_io_num = add_chan_args.cs->ival[0],
                .queue_size = 1,
            };
            spi_device_handle_t handle;
            if (spi_bus_add_device(spi_lines[line].host, &devcfg, &handle) != ESP_OK) {
                printf("spi-channel-add: device add failed\n");
                return 1;
            }
            spi_chans[i] = (spi_channel_t){ .in_use=true, .line_id=(uint8_t)line,
                                         .cs_io=devcfg.spics_io_num, .handle=handle };
            printf("spi-channel-add %d...OK\n", i);
            return 0;
        }
    }
    printf("spi-channel-add: no free channels\n");
    return 1;
}

static int cmd_spi_channel_list(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("SPI Channels:\n");
    for (int i = 0; i < MAX_SPI_CHANNELS; i++) {
        if (spi_chans[i].in_use) {
            printf("  [%d] line=%d CS=%d\n",
                   i,
                   spi_chans[i].line_id,
                   spi_chans[i].cs_io);
        }
    }
    return 0;
}

static int cmd_spi_transfer(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**)&xfer_args)) {
        arg_print_errors(stderr, xfer_args.end, argv[0]);
        return 1;
    }
    int id = xfer_args.chan->ival[0];
    if (id < 0 || id >= MAX_SPI_CHANNELS || !spi_chans[id].in_use) {
        printf("spi-transfer: no such channel %d\n", id);
        return 1;
    }
    const char *hex = xfer_args.data->sval[0];
    size_t len = strlen(hex) / 2;
    uint8_t *tx = malloc(len), *rx = malloc(len);
    for (size_t i = 0; i < len; i++) sscanf(hex + 2*i, "%2hhx", &tx[i]);
    spi_transaction_t t = { .length = len*8, .tx_buffer = tx, .rx_buffer = rx };
    esp_err_t err = spi_device_transmit(spi_chans[id].handle, &t);
    if (err != ESP_OK) {
        printf("spi-transfer: error %d\n", err);
    } else {
        printf("RX:");
        for (size_t i = 0; i < len; i++) printf(" %02X", rx[i]);
        printf("\n");
    }
    free(tx); free(rx);
    return (err == ESP_OK) ? 0 : 1;
}

static int cmd_spi_reset(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**)&reset_args)) {
        arg_print_errors(stderr, reset_args.end, argv[0]);
        return 1;
    }
    gpio_num_t pin = reset_args.pin->ival[0];
    int ms = reset_args.dur->count ? reset_args.dur->ival[0] : 20;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(ms));
    gpio_set_level(pin, 1);
    vTaskDelay(pdMS_TO_TICKS(ms));
    printf("spi-reset on GPIO %d (%d ms)...OK\n", pin, ms);
    return 0;
}

void register_spi(void)
{
    add_line_args.miso = arg_int1(NULL,NULL,"<MISO GPIO>",NULL);
    add_line_args.mosi = arg_int1(NULL,NULL,"<MOSI GPIO>",NULL);
    add_line_args.sck  = arg_int1(NULL,NULL,"<SCK GPIO>", NULL);
    add_line_args.end  = arg_end(3);
    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command  = "spi-line-add",
        .help     = "add SPI bus: spi-line-add <MISO> <MOSI> <SCK>",
        .func     = &cmd_spi_line_add,
        .argtable = &add_line_args
    }) );

    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command = "spi-line-list",
        .help    = "list SPI buses",
        .func    = &cmd_spi_line_list
    }) );

    add_chan_args.line = arg_int1(NULL,NULL,"<line id>",NULL);
    add_chan_args.cs   = arg_int1(NULL,NULL,"<CS GPIO>", NULL);
    add_chan_args.clk  = arg_int1(NULL,NULL,"<clk Hz>",    NULL);
    add_chan_args.end  = arg_end(3);
    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command  = "spi-channel-add",
        .help     = "add device: spi-channel-add <line> <CS> <clk>",
        .func     = &cmd_spi_channel_add,
        .argtable = &add_chan_args
    }) );

    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command = "spi-channel-list",
        .help    = "list SPI devices",
        .func    = &cmd_spi_channel_list
    }) );

    xfer_args.chan = arg_int1(NULL,NULL,"<chan id>",NULL);
    xfer_args.data = arg_str1(NULL,NULL,"<hex bytes>","no spaces");
    xfer_args.end  = arg_end(2);
    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command  = "spi-transfer",
        .help     = "transfer hex data: spi-transfer <chan> <hex>",
        .func     = &cmd_spi_transfer,
        .argtable = &xfer_args
    }) );

    reset_args.pin = arg_int1(NULL,NULL,"<GPIO>","reset pin");
    reset_args.dur = arg_int0(NULL,NULL,"<ms>","pulse duration");
    reset_args.end = arg_end(2);
    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command  = "spi-reset",
        .help     = "pulse reset line: spi-reset <GPIO> [ms]",
        .func     = &cmd_spi_reset,
        .argtable = &reset_args
    }) );
}

spi_device_handle_t spi_get_handle(int chan_id)
{
    return (chan_id >= 0 && chan_id < MAX_SPI_CHANNELS && spi_chans[chan_id].in_use)
           ? spi_chans[chan_id].handle : NULL;
}

gpio_num_t spi_get_cs_gpio(int chan_id)
{
    return (chan_id >= 0 && chan_id < MAX_SPI_CHANNELS && spi_chans[chan_id].in_use)
           ? spi_chans[chan_id].cs_io  : (gpio_num_t)-1;
}

gpio_num_t spi_get_sck_gpio(int chan_id){
  return (chan_id>=0&&chan_id<MAX_SPI_CHANNELS&&spi_chans[chan_id].in_use)?
          spi_lines[spi_chans[chan_id].line_id].sck_io : (gpio_num_t)-1;
}
gpio_num_t spi_get_miso_gpio(int chan_id){
  return (chan_id>=0&&chan_id<MAX_SPI_CHANNELS&&spi_chans[chan_id].in_use)?
          spi_lines[spi_chans[chan_id].line_id].miso_io : (gpio_num_t)-1;
}
gpio_num_t spi_get_mosi_gpio(int chan_id){
  return (chan_id>=0&&chan_id<MAX_SPI_CHANNELS&&spi_chans[chan_id].in_use)?
          spi_lines[spi_chans[chan_id].line_id].mosi_io : (gpio_num_t)-1;
}
