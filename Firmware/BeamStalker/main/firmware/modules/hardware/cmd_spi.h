 #ifndef CMD_SPI_H
 #define CMD_SPI_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

 #ifdef __cplusplus
 extern "C" {
 #endif
 
 void register_spi(void);

spi_device_handle_t spi_get_handle(int chan_id);
gpio_num_t          spi_get_cs_gpio(int chan_id);
gpio_num_t          spi_get_sck_gpio(int chan_id);
gpio_num_t          spi_get_miso_gpio(int chan_id);
gpio_num_t          spi_get_mosi_gpio(int chan_id);

 
 #ifdef __cplusplus
 }
 #endif
 
 #endif
