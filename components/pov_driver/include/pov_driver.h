#pragma once
#include "img_protocol.h"
#include "apa102_driver.h"

void pov_update_strip_task(void *pvParameters);
void pov_update_buffer_task(void *pvParameters);

#ifdef USE_RTOS_QUEUE
typedef struct
{
    apa102_driver_t *led_strips; // 所有的灯条驱动们

    int strip_num;     // 有多少条灯条
    int strip_led_num; // 每个灯条上有多少颗灯

    int pov_div;     // 将一个圆周一圈分成多少个角度去刷新, 数值越大显示约细腻, 但对刷新速度要求更高
    int current_div; //当前转到了第几个div
} pov_driver_t;

pov_driver_t pov_init(void);

/**
 * @brief Update apa102 strip from frame buffer;
 * This function should be called every div
 *
 * @param driver pov driver
 * @return esp_err_t ESP_OK if ok
 */
void pov_update_strip_from_queue(pov_driver_t *driver);

esp_err_t pov_free(pov_driver_t *driver);

#else
#endif
