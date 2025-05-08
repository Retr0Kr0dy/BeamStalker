#pragma once
#include "radio_base.h"
#include "radio_errors.h"

class DriverSX127x final : public RadioDriver {
public:
    DriverSX127x(spi_device_handle_t, gpio_num_t, int, int, int, int) {}
    int16_t begin() override { return RADIOLIB_ERR_NONE; }
    int16_t tx (const RadioConfig&, const uint8_t*, size_t) override { return RADIOLIB_ERR_NONE; }
    int16_t rx (const RadioConfig&, std::vector<uint8_t>&, uint32_t) override { return RADIOLIB_ERR_UNSUPPORTED; }
    const char* name() const override { return "sx127x"; }
};
