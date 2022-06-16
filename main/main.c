#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "pov_driver.h"
#include "tcp_server.h"
#include "mywifi.h"
#include "driver/timer.h"
#include "pov_timer.h"

/*
 * 算了一下, 大概过程是这样:
 * 晶振40MHz, 10分频, 也就是4MHz, 也就是4000kHz, 也就是4 * 1000 * 1000 Hz, 每秒这么多个时钟周期
 * 电机是每分钟3500转, 每秒58.33转, 也l就是4000/58.33 = 68.575 k个时钟周期转一圈
 * 所以我这儿设置成68575
 */
#define ROT_TIME 68575

void app_main(void)
{
    // 连Wifi
    wifi_init_station();

    // 启动TCP Server, TCP Server会handle图片协议的事情, 并且把图片放进消息队列
    tcp_server_task_param_t tcp_param = {
        .addr_family = AF_INET,
        .param = NULL,
    };

    img_queue_init();
    static pov_driver_t pov_driver;
    pov_driver = pov_init();
    ESP_LOGD("main", "Verify the driver info before timer_init: num_strip: %d,  pov_div: %d", pov_driver.strip_num, pov_driver.pov_div);

    pov_timer_init(ROT_TIME, pov_driver.pov_div);

    xTaskCreate(tcp_server_task, "tcp_server", 4096, (void *)&tcp_param, 5, NULL);
    
    xTaskCreate(pov_update_strip_task, "pov_update_strip", 4096, &pov_driver, 1, NULL);

    xTaskCreate(pov_update_buffer_task, "pov_update_buffer", 4096, &pov_driver, 1, NULL);

    pov_timer_start();

    // 启动调度器
    // vTaskStartScheduler();
}