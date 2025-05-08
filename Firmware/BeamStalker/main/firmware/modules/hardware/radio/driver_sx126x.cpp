#include "driver_sx126x.h"

DriverSX126x::DriverSX126x(spi_device_handle_t spi,
                           gpio_num_t          cs,
                           int                 dio1,
                           int                 rst,
                           int                 busy,
                           int                 dio2)
    : _hal (std::make_unique<EspRadioHal>(spi)),
      _mod (std::make_unique<Module>(_hal.get(), cs, dio1, rst, busy)),
      _radio(_mod.get()),
      _dio2 (dio2) {}

int16_t DriverSX126x::begin() {
    RADIOLIB_CHECK(_radio.begin());

    if(_dio2 != RADIOLIB_NC) {
        _radio.setRfSwitchPins(_dio2, _dio2);
        _radio.setDio2AsRfSwitch(true);
    }
    return RADIOLIB_ERR_NONE;
}

int16_t DriverSX126x::tx(const RadioConfig& c,
                         const uint8_t*     data,
                         size_t             len)
{
    if(strcasecmp(c.modulation.c_str(), "lora") == 0) {
        RADIOLIB_CHECK(_radio.begin(c.frequency,
                                    (int)c.bandwidth,
                                    c.sf,
                                    c.cr,
                                    0x12,
                                    c.power));
    } else {
        
        RADIOLIB_CHECK(_radio.beginFSK(c.frequency,
                                        c.bitrate,
                                        /*dev   */50.0,
                                        c.bandwidth > 0 ? c.bandwidth : 125.0,
                                        c.power,
                                        strcasecmp(c.modulation.c_str(),"ook")==0));
    }
    return _radio.transmit(data, len);            
}

int16_t DriverSX126x::rx(const RadioConfig& c,
                         std::vector<uint8_t>& out,
                         uint32_t timeout_ms) {
  
    if(strcasecmp(c.modulation.c_str(), "lora") == 0) {
      RADIOLIB_CHECK(_radio.begin(
        c.frequency,              
        (int)c.bandwidth,         
        c.sf,                     
        c.cr                      
      ));
      
      
    } else {
      RADIOLIB_CHECK(_radio.beginFSK(
        c.frequency,              
        c.bitrate,                
        /*dev*/   50.0,           
        c.bandwidth > 0 ? c.bandwidth : 117.3,  
        /*pwr*/   0,
        /*ook*/   strcasecmp(c.modulation.c_str(), "ook") == 0
      ));
    }

    _radio.startReceive();       
    uint32_t start = _hal->millis();
    
    uint8_t buf[256];
    while(_hal->millis() - start < timeout_ms) {
      if(_radio.available()) {
        
        int16_t n = _radio.readData(buf, sizeof(buf));
        if(n < 0) {
          return n;              
        }
        
        out.assign(buf, buf + n);

        
        printf("  RX → len=%d, RSSI=%.1f dBm, SNR=%.1f dB\n",
              n,
              _radio.getRSSI(),
              _radio.getSNR());

        return RADIOLIB_ERR_NONE;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    
    return RADIOLIB_ERR_RX_TIMEOUT;  
}

int16_t DriverSX126x::scan(float start_MHz,
                           int   step_kHz,   
                           int   count,
                           int   dwell_ms)
{    
    RADIOLIB_CHECK(_radio.begin(start_MHz, 125, 7, 5));


    const float step_MHz = step_kHz * 1e-3f;


    for(int i = 0; i < count; ++i) {
        float f = start_MHz + step_MHz * i;    

        if(step_kHz) {
            _radio.standby();
            RADIOLIB_CHECK(_radio.setFrequency(f));
        }
        
        _radio.startReceive();
        _hal->delay(dwell_ms);

        int16_t rssi = _radio.getRSSI();
        printf("%8lu ms  %.3f MHz  RSSI %4d dBm\n",
                (unsigned long)(i * dwell_ms),
                f,
                rssi);
    }
    _radio.standby();
      return RADIOLIB_ERR_NONE;
}
