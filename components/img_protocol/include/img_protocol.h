#pragma once
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdbool.h>

#define USE_RTOS_QUEUE 1

// RGB值结构体
typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} led_rgb_t;

// 50个rgb值为一个div, 每个灯条转到这里的时候就是一个div
typedef led_rgb_t *div_buffer_t;
// 320个div正好转一圈, 作为一个图像帧
typedef div_buffer_t *img_frame_buffer_t;

/**
 * @brief Create a empty img frame object
 *
 * @return img_frame_buffer_t
 */
img_frame_buffer_t create_empty_img_frame(void);

/**
 * @brief clean up a img frame memory
 *
 * @param frame
 */
void free_img_frame(img_frame_buffer_t frame);

typedef struct
{
    img_frame_buffer_t frame_buffer;
    int current_rgb;
    int current_div;
    int current_led;
    bool is_full;
} temp_frame_buffer_info_t_;

void init_temp_frame_buffer();

void fill_temp_frame_buffer(uint8_t rgb_single_byte);

img_frame_buffer_t update_temp_frame_buffer();


/********使用FreeRTOS自带的消息队列***********/
#ifdef USE_RTOS_QUEUE

/**
 * @brief Init a img frame msg queue
 *
 */
void img_queue_init(void);

BaseType_t push_frame_to_queue(img_frame_buffer_t *frame);
BaseType_t push_frame_to_queue_FromISR(img_frame_buffer_t *frame);

img_frame_buffer_t pop_frame_from_queue();
img_frame_buffer_t pop_frame_from_queue_FromISR(BaseType_t *const high_task_awoken);

img_frame_buffer_t peek_frame_from_queue(void);
img_frame_buffer_t peek_frame_from_queue_FromISR(void);

unsigned int get_queue_waiting_data(void);

#else
/**********************自定义的队列******************/
//  由图像帧组成的链表作为显示使用的缓冲区
typedef struct
{
    img_frame_buffer_t *head;
    img_frame_buffer_t *tail;
    int length;

} * img_buffer_t;

/**
 * @brief 初始化显示图像帧的队列缓冲区
 *
 * @return img_buffer_t 缓冲区队列实例
 */
img_buffer_t img_buffer_init();

/**
 * @brief 入队
 *
 * @param buffer_queue
 * @param frame
 */
void img_buffer_push(img_buffer_t buffer_queue, const img_frame_buffer_t frame);
#endif