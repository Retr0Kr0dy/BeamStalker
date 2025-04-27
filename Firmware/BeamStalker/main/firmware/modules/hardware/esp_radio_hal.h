#ifndef ESP_RADIO_HAL_H
#define ESP_RADIO_HAL_H

#include <RadioLib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#define LOW     0
#define HIGH    1
#define INPUT   GPIO_MODE_INPUT
#define OUTPUT  GPIO_MODE_OUTPUT
#define RISING  GPIO_INTR_POSEDGE
#define FALLING GPIO_INTR_NEGEDGE
#define NOP()   asm volatile("nop")

class EspRadioHal : public RadioLibHal {
 public:
  explicit EspRadioHal(spi_device_handle_t dev)
    : RadioLibHal(INPUT, OUTPUT, LOW, HIGH, RISING, FALLING), _dev(dev) {}

  void init() override {}
  void term() override {}

  void pinMode(uint32_t pin, uint32_t mode) override {
    if(pin == RADIOLIB_NC) return;
    gpio_config_t c{};
    c.pin_bit_mask = 1ULL << pin;
    c.mode = (gpio_mode_t)mode;
    gpio_config(&c);
  }
  void digitalWrite(uint32_t pin, uint32_t val) override {
    if(pin != RADIOLIB_NC) gpio_set_level((gpio_num_t)pin, val);
  }
  uint32_t digitalRead(uint32_t pin) override {
    return pin == RADIOLIB_NC ? 0 : gpio_get_level((gpio_num_t)pin);
  }

  void attachInterrupt(uint32_t pin, void (*cb)(void), uint32_t mode) override {
    if(pin == RADIOLIB_NC) return;
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_set_intr_type((gpio_num_t)pin, (gpio_int_type_t)mode);
    gpio_isr_handler_add((gpio_num_t)pin, (gpio_isr_t)cb, nullptr);
  }
  void detachInterrupt(uint32_t pin) override {
    if(pin == RADIOLIB_NC) return;
    gpio_isr_handler_remove((gpio_num_t)pin);
    gpio_set_intr_type((gpio_num_t)pin, GPIO_INTR_DISABLE);
  }

  void delay(RadioLibTime_t ms)            override { vTaskDelay(ms/portTICK_PERIOD_MS); }
  void delayMicroseconds(RadioLibTime_t us) override { esp_rom_delay_us(us); }
  RadioLibTime_t millis()  override { return esp_timer_get_time() / 1000ULL; }
  RadioLibTime_t micros()  override { return esp_timer_get_time(); }

  long pulseIn(uint32_t pin, uint32_t state, RadioLibTime_t to) override {
    if(pin == RADIOLIB_NC) return 0;
    uint64_t start = esp_timer_get_time();
    while(digitalRead(pin) == state && (esp_timer_get_time() - start) < to*1000ULL) NOP();
    uint64_t mark = esp_timer_get_time();
    while(digitalRead(pin) != state && (esp_timer_get_time() - mark) < to*1000ULL) NOP();
    return (esp_timer_get_time() - mark);
  }

  void spiBegin()            override {}
  void spiBeginTransaction() override {}
  void spiTransfer(uint8_t* out, size_t len, uint8_t* in) override {
    spi_transaction_t t{};
    t.length    = len * 8;
    t.tx_buffer = out;
    t.rx_buffer = in;
    spi_device_polling_transmit(_dev, &t);
  }
  void spiEndTransaction()   override {}
  void spiEnd()              override {}

 private:
  spi_device_handle_t _dev;
};

#endif
