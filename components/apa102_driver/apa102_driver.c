#include "apa102_driver.h"
#include <string.h>
#define TAG "apa102_driver"
apa102_driver_t apa102_init(int leds, spi_host_device_t spi_host, int mosi_pin, int sclk_pin)
{
    
    size_t tx_buffer_size = (leds + 2) * sizeof(uint32_t);

    apa102_driver_t driver = {
        .num_leds = leds,
        .tx_buffer = malloc(tx_buffer_size),
    };
    memset(driver.tx_buffer, 0, tx_buffer_size);
    for (int frame = 0; frame < driver.num_leds; frame++)
    {
        driver.tx_buffer[frame + 1] = 0xE0000000;
    }
    spi_bus_config_t bus_config = {
        .miso_io_num = -1,
        .mosi_io_num = mosi_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_device_interface_config_t dev_config = {
        .clock_speed_hz = 4 * 1000 * 1000,
        .mode = 3,
        .spics_io_num = -1,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(spi_host, &bus_config, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(spi_host, &dev_config, &driver.spi_device));
    return driver;
}

esp_err_t apa102_free(apa102_driver_t driver)
{
    free(driver.tx_buffer);
    return ESP_OK;
}

esp_err_t apa102_set_rgbl(apa102_driver_t driver, int led_index, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness)
{
    if (led_index > driver.num_leds)
    {
        ESP_LOGW(TAG, "Led index %d out of range. Max range is %d. \n", led_index, driver.num_leds);
        return ESP_ERR_INVALID_ARG;
    }
    driver.tx_buffer[led_index] = ((((red)&0xFF) << 24) | (((green)&0xFF) << 16) | (((blue)&0xFF) << 8) | ((((brightness)&0xFF) >> 3)) | 0xE0);
    return ESP_OK;
}

esp_err_t apa102_update(apa102_driver_t driver)
{
    spi_transaction_t trans = {
        .length = 8 * ((2 + driver.num_leds) * sizeof(uint32_t)),
        .tx_buffer = driver.tx_buffer};
    return spi_device_transmit(driver.spi_device, &trans);
}