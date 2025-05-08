#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#define CC1101_CRYSTAL_FREQ  40.0e6
#include <RadioLib.h>

#include "../cmd_spi.h"

struct RadioConfig {
    float      frequency  = 0.0f; 
    float      bandwidth  = 0.0f; 
    std::string modulation;       
    float      bitrate    = 0.0f; 
    int        sf         = 0;    
    int        cr         = 0;    
    int        power      = 0;    
};

class RadioDriver {
public:
    virtual ~RadioDriver() = default;

    virtual int16_t begin()                                   = 0;
    virtual int16_t tx(const RadioConfig& cfg,
                       const uint8_t* data,
                       size_t len)                            = 0;
    virtual int16_t rx(const RadioConfig& cfg,
                       std::vector<uint8_t>& out,
                       uint32_t timeout_ms)                   = 0;

    virtual int16_t scan(float startFreq_Hz,
                         int   step_kHz,
                         int   count,
                         int   dwell_ms) {
        return RADIOLIB_ERR_UNSUPPORTED;
    }

    [[nodiscard]] virtual const char* name() const = 0;
};

std::unique_ptr<RadioDriver> make_driver(const std::string& id,
                                         spi_device_handle_t h,
                                         gpio_num_t cs,
                                         int dio1 = RADIOLIB_NC,
                                         int rst  = RADIOLIB_NC,
                                         int busy = RADIOLIB_NC,
                                         int dio2 = RADIOLIB_NC);
