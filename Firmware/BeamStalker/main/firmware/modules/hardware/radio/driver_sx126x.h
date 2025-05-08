#pragma once
#include <memory>
#include <vector>

#include "radio_base.h"
#include "radio_errors.h"
#include "esp_radio_hal.h"

class DriverSX126x final : public RadioDriver {
public:
    DriverSX126x(spi_device_handle_t spi,
                 gpio_num_t          cs,
                 int                 dio1,
                 int                 rst,
                 int                 busy,
                 int                 dio2);

    int16_t begin() override;
    int16_t tx(const RadioConfig&, const uint8_t*, size_t) override;
    int16_t rx(const RadioConfig&, std::vector<uint8_t>&, uint32_t) override;
    int16_t scan(float start_Hz, int step_kHz, int count, int dwell_ms) override;
    const char* name() const override { return "sx126x"; }

private:
    std::unique_ptr<EspRadioHal> _hal;
    std::unique_ptr<Module>      _mod;
    SX1262                        _radio;
    int                           _dio2;
};
