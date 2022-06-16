#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

SemaphoreHandle_t should_update_strip_div;
SemaphoreHandle_t should_pop_queue_frame;

void pov_timer_init(int rot_time, int pov_div);
void pov_timer_start(void);
bool update_buffer_callback();
bool update_strip_callback();
bool recive_data_callback();