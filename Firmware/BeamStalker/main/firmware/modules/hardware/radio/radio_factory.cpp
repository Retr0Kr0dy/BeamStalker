#include "radio_base.h"

#include "driver_cc1101.h"
#include "driver_sx126x.h"
#include "driver_sx127x.h"

std::unique_ptr<RadioDriver> make_driver(const std::string& id,
                                         spi_device_handle_t h,
                                         gpio_num_t          cs,
                                         int dio1,
                                         int rst,
                                         int busy,
                                         int dio2)
{
    if (id == "cc1101" || id == "cc110x") {
        return std::make_unique<DriverCC1101>(h, cs, dio1, rst, busy, dio2);
    }
    if (id == "sx126x" || id == "sx1262" || id == "sx1261") {
        return std::make_unique<DriverSX126x>(h, cs, dio1, rst, busy, dio2);
    }
    if (id == "sx127x" || id == "rfm95" || id == "rfm96") {
        return std::make_unique<DriverSX127x>(h, cs, dio1, rst, busy, dio2);
    }
    return nullptr;
}
