#include <stdio.h>
#include "pov_driver.h"
#include "pov_timer.h"

#define TAG "pov_driver"

#ifdef USE_RTOS_QUEUE

pov_driver_t pov_init(void)
{
    int strip_led_num_ = 50; // 每条灯条上有多少颗灯
    int strip_num_ = 2;      // 有多少条灯条
    int pov_div_ = 320;      // 将一个圆周一圈分成多少个角度去刷新, 数值越大显示越细腻, 但对刷新速度要求更高

    pov_driver_t driver = {
        .led_strips = (apa102_driver_t *)calloc(strip_num_, sizeof(apa102_driver_t)),
        .pov_div = pov_div_,
        .strip_num = strip_num_,
        .current_div = 0,
    };

    driver.led_strips[0] = apa102_init(strip_led_num_, HSPI_HOST, 33, 32);
    driver.led_strips[1] = apa102_init(strip_led_num_, VSPI_HOST, 23, 18);

    // rgb * strip_led_num * num_strip * pov_div * frame_buffer_num
    // we totally has multiple frame

    // create a Semaphore to recive notify from timer

    ESP_LOGD(TAG, "Pov init finished");
    return driver;
}

void pov_update_strip_task(void *pvParameters)
{
    ESP_LOGD(TAG, "Start POV update strip task");

    pov_driver_t *driver = (pov_driver_t *)pvParameters;
    while (true)
    {
        if (xSemaphoreTake(should_update_strip_div, portMAX_DELAY) == pdTRUE)
        {
            pov_update_strip_from_queue(driver);
        }
    }
}

void pov_update_buffer_task(void *pvParameters)
{
    ESP_LOGD(TAG, "Start POV update buffer task");
    pov_driver_t *driver = (pov_driver_t *)pvParameters;

    while (true)
    {
        if (xSemaphoreTake(should_pop_queue_frame, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGD(TAG, "Pop a frame from queue for update buffer");
            pop_frame_from_queue();
            driver->current_div = 0;
        }
    }
}

void pov_update_strip_from_queue(pov_driver_t *driver)
{

    unsigned int queue_waiting_data = get_queue_waiting_data();
    ESP_LOGD(TAG, "Update strip from queue, there are %d data in the queue. ", queue_waiting_data);

    img_frame_buffer_t frame = peek_frame_from_queue();
    ESP_LOGD(TAG, "Got a frame from queue");
    int oppsite_div_index = 0;
    ESP_LOGD(TAG, "pov_div: %d, strip_num: %d", driver->pov_div, driver->strip_num);
    if (driver->current_div > (driver->pov_div / driver->strip_num))
    {
        oppsite_div_index = (driver->pov_div / driver->strip_num) - driver->current_div;
    }
    else
    {
        oppsite_div_index = (driver->pov_div / driver->strip_num) + driver->current_div;
    }

    div_buffer_t div = frame[driver->current_div];
    div_buffer_t oppsite_div = frame[oppsite_div_index];

    for (size_t led = 0; led < driver->strip_led_num; led++)
    {
        ESP_ERROR_CHECK(apa102_set_rgbl(driver->led_strips[0], led, div[led].red, div[led].green, div[led].blue, 12));
        ESP_ERROR_CHECK(apa102_set_rgbl(driver->led_strips[1], led, oppsite_div[led].red, oppsite_div[led].green, oppsite_div[led].blue, 12));
    }
    ESP_LOGD(TAG, "Updateing apa102 strip index %d", 0);
    ESP_ERROR_CHECK(apa102_update(driver->led_strips[0]));
    ESP_LOGD(TAG, "Updateing apa102 strip index %d", 1);
    ESP_ERROR_CHECK(apa102_update(driver->led_strips[1]));
    ESP_LOGD(TAG, "Update all strips OK");
    driver->current_div++;
}

#else
#endif