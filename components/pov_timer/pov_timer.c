#include <stdio.h>
#include "pov_timer.h"
#include "driver/timer.h"
#include <esp_log.h>

// static const char *TAG = "pov_timer";
#define TAG ("pov_timer")

void pov_timer_init(int rot_time, int pov_div)
{
    should_update_strip_div = xSemaphoreCreateBinary();
    should_pop_queue_frame = xSemaphoreCreateBinary();

    timer_config_t rot_timer_config = {
        .divider = 10,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = TIMER_AUTORELOAD_EN,
    };

    ESP_LOGD(TAG, "POV Timer Init...");
    ESP_ERROR_CHECK(timer_init(TIMER_GROUP_1, TIMER_0, &rot_timer_config));
    ESP_ERROR_CHECK(timer_init(TIMER_GROUP_1, TIMER_1, &rot_timer_config));

    ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0));
    ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP_1, TIMER_1, 0));

    // 给每转一圈注册中断
    ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP_1, TIMER_0, rot_time));
    ESP_ERROR_CHECK(timer_enable_intr(TIMER_GROUP_1, TIMER_0));
    ESP_ERROR_CHECK(timer_isr_callback_add(TIMER_GROUP_1, TIMER_0, (timer_isr_t)update_buffer_callback, (void *)NULL, 0));

    // 给每次灯条刷新注册中断
    ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP_1, TIMER_1, rot_time / pov_div));
    ESP_ERROR_CHECK(timer_enable_intr(TIMER_GROUP_1, TIMER_1));
    ESP_ERROR_CHECK(timer_isr_callback_add(TIMER_GROUP_1, TIMER_1, (timer_isr_t)update_strip_callback, (void *)NULL, 0));
}

bool update_buffer_callback(void *any)
{
    timer_set_alarm(TIMER_GROUP_1, TIMER_0, TIMER_ALARM_EN);
    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(should_pop_queue_frame, &high_task_awoken);
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

bool update_strip_callback(void *any)
{
    timer_set_alarm(TIMER_GROUP_1, TIMER_1, TIMER_ALARM_EN);
    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(should_update_strip_div, &high_task_awoken);
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

bool recive_data_callback(void *any)
{
    BaseType_t high_task_awoken = pdFALSE;
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

void pov_timer_start(void)
{
    ESP_LOGD(TAG, "POV Timer start...");

    ESP_ERROR_CHECK(timer_set_alarm(TIMER_GROUP_1, TIMER_0, TIMER_ALARM_EN));
    ESP_ERROR_CHECK(timer_set_alarm(TIMER_GROUP_1, TIMER_1, TIMER_ALARM_EN));
    ESP_ERROR_CHECK(timer_start(TIMER_GROUP_1, TIMER_0));
    ESP_ERROR_CHECK(timer_start(TIMER_GROUP_1, TIMER_1));
}