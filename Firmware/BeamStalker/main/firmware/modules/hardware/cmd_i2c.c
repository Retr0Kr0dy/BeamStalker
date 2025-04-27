#include "cmd_i2c.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "driver/i2c.h"
#include <stdio.h>

#define MAX_LINES     4
#define MAX_CHANNELS 16

typedef struct {
    bool        in_use;
    gpio_num_t  sda_io;
    gpio_num_t  scl_io;
    i2c_port_t  port;
} i2c_line_t;

typedef struct {
    bool        in_use;
    uint8_t     line_id;
    uint16_t    address;
    i2c_port_t  port;
} i2c_channel_t;

static i2c_line_t    lines[MAX_LINES] = {0};
static i2c_channel_t channels[MAX_CHANNELS] = {0};

static struct {
    struct arg_int *sda, *scl;
    struct arg_end *end;
} add_line_args;

static struct {
    struct arg_int *line;
    struct arg_end *end;
} scan_args;

static struct {
    struct arg_int *line, *addr;
    struct arg_end *end;
} add_chan_args;

static int cmd_i2c_line_add(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**)&add_line_args)) {
        arg_print_errors(stderr, add_line_args.end, argv[0]);
        return 1;
    }
    for (int i = 0; i < MAX_LINES; i++) {
        if (!lines[i].in_use) {
            gpio_num_t sda = add_line_args.sda->ival[0];
            gpio_num_t scl = add_line_args.scl->ival[0];
            i2c_config_t cfg = {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = sda,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_io_num = scl,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master.clk_speed = 100000
            };
            i2c_param_config(i, &cfg);
            i2c_driver_install(i, cfg.mode, 0, 0, 0);
            lines[i] = (i2c_line_t){ .in_use = true, .sda_io = sda, .scl_io = scl, .port = i };
            printf("i2c-line-add %d...OK\n", i);
            return 0;
        }
    }
    printf("i2c-line-add: no free lines\n");
    return 1;
}

static int cmd_i2c_line_list(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("I2C Lines:\n");
    for (int i = 0; i < MAX_LINES; i++) {
        if (lines[i].in_use) {
            printf("  [%d] SDA=%d SCL=%d\n", i, lines[i].sda_io, lines[i].scl_io);
        }
    }
    return 0;
}

static int cmd_i2c_scan(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**)&scan_args)) {
        arg_print_errors(stderr, scan_args.end, argv[0]);
        return 1;
    }
    int id = scan_args.line->ival[0];
    if (id < 0 || id >= MAX_LINES || !lines[id].in_use) {
        printf("i2c-scan: invalid line %d\n", id);
        return 1;
    }
    printf("Scanning on line %d:", id);
    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(lines[id].port, cmd, 20 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (err == ESP_OK) {
            printf(" 0x%02X", addr);
        }
    }
    printf("\n");
    return 0;
}

static int cmd_i2c_channel_add(int argc, char **argv)
{
    if (arg_parse(argc, argv, (void**)&add_chan_args)) {
        arg_print_errors(stderr, add_chan_args.end, argv[0]);
        return 1;
    }
    int line = add_chan_args.line->ival[0];
    int addr = add_chan_args.addr->ival[0] & 0x7F;
    if (line < 0 || line >= MAX_LINES || !lines[line].in_use) {
        printf("i2c-channel-add: invalid line %d\n", line);
        return 1;
    }
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (!channels[i].in_use) {
            channels[i] = (i2c_channel_t){
                .in_use   = true,
                .line_id  = line,
                .address  = addr,
                .port     = lines[line].port
            };
            printf("i2c-channel-add %d...OK\n", i);
            return 0;
        }
    }
    printf("i2c-channel-add: no free channels\n");
    return 1;
}

static int cmd_i2c_channel_list(int argc, char **argv)
{
    (void)argc; (void)argv;
    printf("I2C Channels:\n");
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i].in_use) {
            printf("  [%d] line=%d addr=0x%02X\n",
                   i, channels[i].line_id, channels[i].address);
        }
    }
    return 0;
}

void register_i2c(void)
{
    add_line_args.sda = arg_int1(NULL, NULL, "<SDA GPIO>", NULL);
    add_line_args.scl = arg_int1(NULL, NULL, "<SCL GPIO>", NULL);
    add_line_args.end = arg_end(2);
    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command  = "i2c-line-add",
        .help     = "add new I2C line: i2c-line-add <SDA> <SCL>",
        .func     = &cmd_i2c_line_add,
        .argtable = &add_line_args
    }) );

    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command = "i2c-line-list",
        .help    = "list I2C lines",
        .func    = &cmd_i2c_line_list
    }) );

    scan_args.line = arg_int1(NULL, NULL, "<line id>", NULL);
    scan_args.end  = arg_end(1);
    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command  = "i2c-scan",
        .help     = "scan I2C bus on a line: i2c-scan <line>",
        .func     = &cmd_i2c_scan,
        .argtable = &scan_args
    }) );

    add_chan_args.line = arg_int1(NULL, NULL, "<line id>", NULL);
    add_chan_args.addr = arg_int1(NULL, NULL, "<addr(7bit)>", NULL);
    add_chan_args.end  = arg_end(2);
    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command  = "i2c-channel-add",
        .help     = "bind address to channel: i2c-channel-add <line> <addr>",
        .func     = &cmd_i2c_channel_add,
        .argtable = &add_chan_args
    }) );

    ESP_ERROR_CHECK( esp_console_cmd_register(&(esp_console_cmd_t){
        .command = "i2c-channel-list",
        .help    = "list I2C channels",
        .func    = &cmd_i2c_channel_list
    }) );
}
