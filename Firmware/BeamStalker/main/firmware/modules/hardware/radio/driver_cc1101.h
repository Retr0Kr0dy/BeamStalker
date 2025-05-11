#pragma once
#include "radio_base.h"     
#include "esp_radio_hal.h"
#include "radio_errors.h"   

class CC1101_Hack : public CC1101 {
public:
    using CC1101::SPIsendCommand;
    using CC1101::SPIreadRegister;
    using CC1101::SPIwriteRegister;
    using CC1101::available;
    using CC1101::startReceive;
    using CC1101::receive;
    using CC1101::readData;
    using CC1101::reset;

    explicit CC1101_Hack(Module* m) : CC1101(m) {}
};

class DriverCC1101 final : public RadioDriver {
public:
    DriverCC1101(spi_device_handle_t spi,
                 gpio_num_t          cs,
                 int                 dio1,
                 int                 rst,
                 int                 busy,
                 int                 dio2);
    int16_t begin() override;
    int16_t tx (const RadioConfig&, const uint8_t*, size_t) override;
    int16_t rx (const RadioConfig&, std::vector<uint8_t>&, uint32_t) override;
    int16_t scan(float start_Hz, int step_kHz, int count, int dwell_ms) override;
    const char* name() const override { return "cc1101"; }
    int16_t rxStop();


private:
    std::unique_ptr<EspRadioHal> _hal;
    std::unique_ptr<Module>      _mod;
    CC1101_Hack                  _radio;
    int                          _dio1Pin;
    static void sniffTask(void* arg);
};
