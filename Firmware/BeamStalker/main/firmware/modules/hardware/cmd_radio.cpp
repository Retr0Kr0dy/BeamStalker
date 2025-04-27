#include <cstring>
#include <cstdlib>
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include "esp_err.h"
#include <RadioLib.h>
#include "esp_radio_hal.h"

extern "C" {
#include "cmd_spi.h"
}

struct RadioSlot {
    bool            used{};
    int             spi_chan{};
    EspRadioHal    *hal{};
    Module         *mod{};
    PhysicalLayer  *iface{};
    char           *name{};
};
static constexpr int MAX_RADIOS = 4;
static RadioSlot radios[MAX_RADIOS]{};

static struct {
    arg_int *chan;
    arg_str *drv;
    arg_int *dio1,*rst,*busy;
    struct arg_end *end;
} a;

static PhysicalLayer* make_iface(const char *d, Module *m, int16_t &st)
{
    if(!strcasecmp(d,"sx126x")||!strcasecmp(d,"sx1262")||!strcasecmp(d,"sx1261")){
        auto *p = new SX1262(m); st = p->begin(); return p;
    }
    if(!strcasecmp(d,"sx127x")||!strcasecmp(d,"rfm95")||!strcasecmp(d,"rfm96")){
        auto *p = new SX1278(m); st = p->begin(); return p;
    }
    if(!strcasecmp(d,"cc1101") || !strcasecmp(d,"cc110x")) {
        auto *p = new CC1101(m); st = p->begin(); return p;
    }

    st = RADIOLIB_ERR_UNKNOWN; return nullptr;
    
}

static int cmd_radio_init(int argc,char**argv)
{
    if(arg_parse(argc,argv,(void**)&a)){
        arg_print_errors(stderr,a.end,argv[0]); return 1;
    }
    int chan = a.chan->ival[0];
    const char *drv = a.drv->sval[0];

    spi_device_handle_t h = spi_get_handle(chan);
    gpio_num_t nss = spi_get_cs_gpio(chan);
    if(!h || nss==(gpio_num_t)-1){ printf("radio-init: bad spi %d\n",chan); return 1; }

    int dio1 = a.dio1->count? a.dio1->ival[0] : RADIOLIB_NC;
    int rst  = a.rst ->count? a.rst ->ival[0] : RADIOLIB_NC;
    int busy = a.busy->count? a.busy->ival[0] : RADIOLIB_NC;

    int slot=-1; for(int i=0;i<MAX_RADIOS;i++) if(!radios[i].used){slot=i;break;}
    if(slot<0){ puts("radio-init: no free slots"); return 1; }

    auto *hal = new EspRadioHal(h);
    auto *mod = new Module(hal,nss,dio1,rst,busy);

    int16_t st; PhysicalLayer *iface = make_iface(drv,mod,st);
    if(!iface){ printf("radio-init: unknown %s\n",drv); return 1; }
    if(st!=RADIOLIB_ERR_NONE){ printf("radio-init: begin %d\n",st); return 1; }

    radios[slot].used     = true;
    radios[slot].spi_chan = chan;
    radios[slot].hal      = hal;
    radios[slot].mod      = mod;
    radios[slot].iface    = iface;
    radios[slot].name     = strdup(drv);

    printf("radio-init: slot %d → %s on SPI %d OK\n",slot,drv,chan);
    return 0;
}

static int cmd_radio_list(int, char**){
    puts("Active radios:");
    for(int i=0;i<MAX_RADIOS;i++) if(radios[i].used)
        printf("  [%d] %s (spi=%d)\n",i,radios[i].name,radios[i].spi_chan);
    return 0;
}

static inline SX126x* as126(int slot){
    return (!strncasecmp(radios[slot].name,"sx126",5))?
            static_cast<SX126x*>(radios[slot].iface):nullptr;
}
static inline SX1278* as127(int slot){
    return ((!strncasecmp(radios[slot].name,"sx127",5))||
            (!strncasecmp(radios[slot].name,"rfm9",4)))?
            static_cast<SX1278*>(radios[slot].iface):nullptr;
}
static CC1101* ascc(int slot){
    return (!strcasecmp(radios[slot].name,"cc1101")) ?
            static_cast<CC1101*>(radios[slot].iface) : nullptr;
}

static int cmd_cc_set(int argc,char**argv){
    if(argc<4){ puts("usage: radio-cc-set <slot> freq|bitrate|rxbw|ook <val>"); return 1; }
    int slot=atoi(argv[1]); auto* cc=ascc(slot); if(!cc){ puts("not CC1101"); return 1; }

    const char* f=argv[2]; float v=atof(argv[3]);
    int16_t st=-99;
    if(!strcmp(f,"freq"))    st=cc->setFrequency(v*1e6);
    else if(!strcmp(f,"bitrate")) st=cc->setBitRate(v);
    else if(!strcmp(f,"rxbw"))    st=cc->setRxBandwidth(v);
    else if(!strcmp(f,"ook"))     st=cc->setOOK(v!=0);
    else { puts("field?"); return 1; }

    printf("%s -> %d\n", f, st); return 0;
}

static int cmd_cc_rx(int argc,char**argv){
    if(argc<2){ puts("usage: radio-cc-rx <slot>"); return 1; }
    int slot=atoi(argv[1]); auto* cc=ascc(slot); if(!cc){ puts("not CC1101"); return 1; }

    uint8_t buf[64]; int16_t len=cc->receive(buf,sizeof(buf));
    if(len<0){ printf("timeout (%d)\n", len); return 0; }
    printf("RX (%d B): ",len); for(int i=0;i<len;i++) printf("%02X ",buf[i]);
    printf(" RSSI=%.1f dBm\n", cc->getRSSI()); return 0;
}

static int cmd_radio_tx(int argc,char**argv)
{
    if(argc<8){ puts("usage: radio-tx <slot> <MHz> <sf> <bw-k> <cr> <pwr> <hex>"); return 1; }
    int slot=atoi(argv[1]); if(slot>=MAX_RADIOS||!radios[slot].used){puts("no such slot");return 1;}

    float   freq = atof(argv[2])*1e6;
    uint8_t sf   = atoi(argv[3]);
    float   bw   = atof(argv[4])*1e3;
    uint8_t cr   = atoi(argv[5]);
    int8_t  pwr  = atoi(argv[6]);
    const char*hex=argv[7];

    size_t len=strlen(hex)/2; if(len>255){puts("pkt too long");return 1;}
    uint8_t buf[255]; for(size_t i=0;i<len;i++) sscanf(hex+2*i,"%2hhx",&buf[i]);

    if(auto*p=as126(slot)){
        p->standby(); p->setFrequency(freq); p->setBandwidth(bw);
        p->setSpreadingFactor(sf); p->setCodingRate(cr); p->setOutputPower(pwr);
        printf("transmit -> %d\n", p->transmit(buf,len)); return 0;
    }
    if(auto*p=as127(slot)){
        p->standby(); p->setFrequency(freq); p->setBandwidth(bw);
        p->setSpreadingFactor(sf); p->setCodingRate(cr); p->setOutputPower(pwr);
        printf("transmit -> %d\n", p->transmit(buf,len)); return 0;
    }
    puts("driver not LoRa family"); return 1;
}

static int cmd_radio_rx(int argc,char**argv)
{
    if(argc<6){ puts("usage: radio-rx <slot> <MHz> <sf> <bw-k> <cr>"); return 1; }
    int slot=atoi(argv[1]); if(slot>=MAX_RADIOS||!radios[slot].used){puts("no such slot");return 1;}

    float   freq = atof(argv[2])*1e6;
    uint8_t sf   = atoi(argv[3]);
    float   bw   = atof(argv[4])*1e3;
    uint8_t cr   = atoi(argv[5]);

    uint8_t buf[255]; int16_t len=-1; float rssi=0,snr=0;

    if(auto*p=as126(slot)){
        p->standby(); p->setFrequency(freq); p->setBandwidth(bw);
        p->setSpreadingFactor(sf); p->setCodingRate(cr);
        len=p->receive(buf,sizeof(buf)); rssi=p->getRSSI(); snr=p->getSNR();
    }else if(auto*p=as127(slot)){
        p->standby(); p->setFrequency(freq); p->setBandwidth(bw);
        p->setSpreadingFactor(sf); p->setCodingRate(cr);
        len=p->receive(buf,sizeof(buf)); rssi=p->getRSSI(); snr=p->getSNR();
    }else{ puts("driver not LoRa family"); return 1; }

    if(len<0){ printf("receive -> %d\n",len); return 0; }

    printf("RX (%d B): ",len); for(int i=0;i<len;i++) printf("%02X",buf[i]);
    printf("  RSSI=%.1f dBm  SNR=%.1f dB\n",rssi,snr);
    return 0;
}

extern "C" void register_radio(void)
{
    a.chan=arg_int1(nullptr,nullptr,"<spi>","channel id");
    a.drv =arg_str1(nullptr,nullptr,"<drv>","sx126x|sx127x|…");
    a.dio1=arg_int0(nullptr,nullptr,"dio1","IRQ GPIO");
    a.rst =arg_int0(nullptr,nullptr,"rst","reset GPIO");
    a.busy=arg_int0(nullptr,nullptr,"busy","busy GPIO");
    a.end =arg_end(5);

    esp_console_cmd_t cmd{};
    cmd.command="radio-init"; cmd.help="radio-init …"; cmd.func=&cmd_radio_init; cmd.argtable=&a;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd={}; cmd.command="radio-list"; cmd.help="list radios"; cmd.func=&cmd_radio_list;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd={}; cmd.command="radio-tx"; cmd.help="see usage"; cmd.func=&cmd_radio_tx;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd={}; cmd.command="radio-rx"; cmd.help="see usage"; cmd.func=&cmd_radio_rx;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd={}; cmd.command="radio-cc-set"; cmd.help="quick CC1101 param"; cmd.func=&cmd_cc_set;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    cmd={}; cmd.command="radio-cc-rx"; cmd.help="blocking CC1101 RX"; cmd.func=&cmd_cc_rx;
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
