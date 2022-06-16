#pragma once
#include <esp_system.h>
#include <esp_log.h>
#include <driver/spi_master.h>

typedef struct
{
    spi_device_handle_t spi_device;
    int num_leds;
    uint32_t *tx_buffer;
} apa102_driver_t;

apa102_driver_t apa102_init(int leds, spi_host_device_t spi_host, int mosi_pin, int sclk_pin);
esp_err_t apa102_free(apa102_driver_t driver);
esp_err_t apa102_set_rgbl(apa102_driver_t driver, int led_index, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
esp_err_t apa102_update(apa102_driver_t driver);
