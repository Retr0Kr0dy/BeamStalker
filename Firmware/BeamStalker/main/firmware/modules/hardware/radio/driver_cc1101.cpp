#include "driver_cc1101.h"
#include "radio_errors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "../../system/baatsh/signal_ctrl.h"

DriverCC1101::DriverCC1101(spi_device_handle_t spi,
                        gpio_num_t          cs,
                        int                 dio1,
                        int                 rst,
                        int                 busy,
                        int                 dio2)
    : _hal (std::make_unique<EspRadioHal>(spi)),
        _mod (std::make_unique<Module>(_hal.get(), cs, dio1, rst, dio2)),
        _radio(_mod.get()),
        _dio1Pin(dio1) {}
        
    int16_t DriverCC1101::begin()
    {
        int16_t st = _radio.begin();                 
        if(st != RADIOLIB_ERR_NONE) return st;

        _radio.reset();                              
        _hal->delay(1);
        _radio.SPIsendCommand(0x33);                 
        _radio.SPIwriteRegister(RADIOLIB_CC1101_REG_IOCFG0, 0x06);
        return RADIOLIB_ERR_NONE;
}

int16_t DriverCC1101::tx(const RadioConfig& c,
                        const uint8_t* data,
                        size_t len) {
    int16_t st;
    
    st = _radio.begin(c.frequency,
                        c.bitrate,              
                        c.bitrate * 0.5,        
                        c.bandwidth,            
                        c.power,                
                        /*preamble*/ 16);       
    if(st != RADIOLIB_ERR_NONE) {
        printf("[TX] begin() failed → %s (%d)\n", radiolib_strerror(st), st);
        return st;
    }

    
    if(strcasecmp(c.modulation.c_str(), "ook") == 0) {
        st = _radio.setOOK(true);
        if(st != RADIOLIB_ERR_NONE) {
        printf("[TX] setOOK() failed → %s (%d)\n", radiolib_strerror(st), st);
        return st;
        }
    }
    
    st = _radio.transmit(data, len);
    printf("[TX] transmit() → %s (%d)\n", radiolib_strerror(st), st);
    return st;
}

int16_t DriverCC1101::rx(const RadioConfig& c,
                        std::vector<uint8_t>& out,
                        uint32_t /*timeout_ms—ignored*/)
{
    int16_t st;
    
    st = _radio.begin(
        c.frequency,               
        c.bitrate,                 
        c.bitrate * 0.5,           
        c.bandwidth,               
        /*power*/ 0,               
        /*preamble*/ 16            
    );
    if(st != RADIOLIB_ERR_NONE) {
        printf("[RX] begin() failed → %s (%d)\n", radiolib_strerror(st), st);
        return st;
    }

    if(strcasecmp(c.modulation.c_str(), "ook") == 0) {
        st = _radio.setOOK(true);
        if(st != RADIOLIB_ERR_NONE) {
        printf("[RX] setOOK() failed → %s (%d)\n", radiolib_strerror(st), st);
        return st;
        }
    }

    
    
    

    
    st = _radio.setEncoding(RADIOLIB_ENCODING_NRZ);
    if(st != RADIOLIB_ERR_NONE) {
        printf("[RX] setEncoding() failed → %s (%d)\n", radiolib_strerror(st), st);
        return st;
    }

    
    _radio.SPIsendCommand(RADIOLIB_CC1101_CMD_FLUSH_RX);
    _hal->delay(1);

    
    uint8_t buf[256];
    printf("[RX] waiting for packet...\n");
    int16_t len = _radio.receive(buf, sizeof(buf));
    if(len < 0) {
        printf("[RX] receive() → %s (%d)\n", radiolib_strerror(len), len);
        return len;
    }

    
    out.assign(buf, buf + len);
    printf("[RX] got %d bytes →", len);
    for(int i = 0; i < len; i++) {
        printf(" %02X", buf[i]);
    }
    printf("   RSSI=%.1f dBm  LQI=%.1d\n", _radio.getRSSI(), _radio.getLQI());

    return RADIOLIB_ERR_NONE;
}

int16_t DriverCC1101::scan(float start_Hz,
                        int step_kHz,
                        int count,
                        int dwell_ms)
{
    for(int i = 0; i < count; ++i) {
        float f = start_Hz + step_kHz * 1e3f * i;
        _radio.SPIsendCommand(0x36);             
        _radio.SPIsendCommand(0x3A);             
        RADIOLIB_CHECK(_radio.setFrequency(f));
        _radio.SPIsendCommand(0x34);             
        _hal->delay(dwell_ms);
        int16_t rssi = _radio.getRSSI();         
        printf("  %.3f MHz → RSSI %d dBm\n", f/1e6f, rssi);
    }
    _radio.SPIsendCommand(0x36);                 
    return RADIOLIB_ERR_NONE;
}
