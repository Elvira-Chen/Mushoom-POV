#include <stdlib.h>
#include "img_protocol.h"
#include "esp_log.h"
#define TAG "img_protocol"

static QueueHandle_t img_buffer_queue;

img_frame_buffer_t create_empty_img_frame(void)
{
    img_frame_buffer_t frame = (div_buffer_t *)calloc(320, sizeof(div_buffer_t));
    for (size_t i = 0; i < 320; i++)
    {
        frame[i] = (led_rgb_t *)calloc(50, sizeof(led_rgb_t));
    }
    return frame;
}

void free_img_frame(img_frame_buffer_t frame)
{
    for (size_t i = 0; i < 320; i++)
    {
        free(frame[i]);
    }
    free(frame);
}

static temp_frame_buffer_info_t_ temp_frame_buffer;
void init_temp_frame_buffer()
{
    temp_frame_buffer_info_t_ buffer = {
        .current_div = 0,
        .current_led = 0,
        .current_rgb = 0,
        .is_full = false,
        .frame_buffer = create_empty_img_frame(),
    };
    temp_frame_buffer = buffer;
}

void fill_temp_frame_buffer(uint8_t rgb_single_byte)
{
    switch (temp_frame_buffer.current_rgb)
    {
    case 0:
    {
        temp_frame_buffer.frame_buffer[temp_frame_buffer.current_div][temp_frame_buffer.current_led].red = rgb_single_byte;
        temp_frame_buffer.current_rgb++;
    }
    break;
    case 1:
    {
        temp_frame_buffer.frame_buffer[temp_frame_buffer.current_div][temp_frame_buffer.current_led].green = rgb_single_byte;
        temp_frame_buffer.current_rgb++;
    }
    break;
    case 2:
    {
        temp_frame_buffer.frame_buffer[temp_frame_buffer.current_div][temp_frame_buffer.current_led].blue = rgb_single_byte;
        temp_frame_buffer.current_rgb = 0;
        temp_frame_buffer.current_led++;
        if (temp_frame_buffer.current_led == 50)
        {
            temp_frame_buffer.current_led = 0;
            temp_frame_buffer.current_div++;
            if (temp_frame_buffer.current_div == 320)
            {
                temp_frame_buffer.current_div = 0;
                temp_frame_buffer.is_full = true;
            }
        }
    }
    break;
    default:
        break;
    }
}

img_frame_buffer_t update_temp_frame_buffer()
{
    if (temp_frame_buffer.is_full)
    {
        img_frame_buffer_t frame = temp_frame_buffer.frame_buffer;
        temp_frame_buffer.frame_buffer = create_empty_img_frame();
        temp_frame_buffer.is_full = false;
        return frame;
    }
    return NULL;
}

#ifdef USE_RTOS_QUEUE
void img_queue_init()
{
    img_buffer_queue = xQueueCreate(10, sizeof(img_frame_buffer_t));
}

BaseType_t push_frame_to_queue(img_frame_buffer_t *frame)
{
    ESP_LOGD(TAG, "Pushing frame to queue");
    xQueueSend(img_buffer_queue, frame, portMAX_DELAY);
    return pdFALSE;
}

BaseType_t push_frame_to_queue_FromISR(img_frame_buffer_t *frame)
{
    BaseType_t high_task_awoken = pdFALSE;
    xQueueSendFromISR(img_buffer_queue, frame, &high_task_awoken);
    return high_task_awoken == pdTRUE; // return whether we need to yield at the end of ISR
}

img_frame_buffer_t pop_frame_from_queue()
{
    ESP_LOGD(TAG, "Pop frame from queue...");
    img_frame_buffer_t frame = create_empty_img_frame();
    xQueueReceive(img_buffer_queue, &frame, portMAX_DELAY);
    return frame;
}

img_frame_buffer_t pop_frame_from_queue_FromISR(BaseType_t *const high_task_awoken)
{
    img_frame_buffer_t frame = create_empty_img_frame();
    BaseType_t res = xQueueReceiveFromISR(img_buffer_queue, &frame, high_task_awoken);
    if (res == errQUEUE_EMPTY)
    {
        return NULL;
    }
    else
    {
        return frame;
    }
    return frame;
}

img_frame_buffer_t peek_frame_from_queue()
{
    img_frame_buffer_t frame;
    xQueuePeek(img_buffer_queue, &frame, portMAX_DELAY);
    return frame;
}

img_frame_buffer_t peek_frame_from_queue_FromISR()
{
    img_frame_buffer_t frame;
    BaseType_t res = xQueuePeekFromISR(img_buffer_queue, &frame);
    if (res == errQUEUE_EMPTY)
    {
        return NULL;
    }
    else
    {
        return frame;
    }
}

unsigned int get_queue_waiting_data()
{
    return uxQueueMessagesWaiting(img_buffer_queue);
}

#else
#endif