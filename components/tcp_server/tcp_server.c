#include <stdio.h>
#include "tcp_server.h"
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "img_protocol.h"
#include "nvs_flash.h"

#define PORT 3333
// Keep-alive idle time.
// 如果超过这个时间还没收到任何数据就发送Keep-alive检查数据包
#define KEEPALIVE_IDLE 5
// Keep-alive probe packet 间隔时间
#define KEEPALIVE_INTERVAL 5
// Keep-alive probe packet 重试次数
#define KEEPALIVE_COUNT 5

#define ENABLED_IPV4 1

static const char *TAG = "tcp_server";

static void recive_data(const int sock);

void tcp_server_task(void *pvParameters)
{
    ESP_LOGD(TAG, "Start TCP Server TASK..");
    tcp_server_task_param_t *param = (tcp_server_task_param_t *)pvParameters;

    char addr_str[128];
    int addr_family = param->addr_family;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;
    if (addr_family == AF_INET)
    {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }
    ESP_LOGD(TAG, "value setted");

#ifdef ENABLED_IPV6
    else if (addr_family == AF_INET6)
    {
        ESP_LOGI("IPv6 Enabled");
        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        ip_protocol = IPPROTO_IPV6;
    }
#endif
    ESP_LOGD(TAG, "start create listen_sock");
    ESP_LOGD(TAG, "ip_protocol: %d", ip_protocol);
    ESP_LOGD(TAG, "Addr_family: %d", addr_family);
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    ESP_LOGD(TAG, "Listen Sock has been created");
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    ESP_LOGD(TAG, "Start set sock opt");
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGD(TAG, "Set sock opt finished");
#if defined(ENABLED_IPV4) && defined(ENABLED_IPV6)
    // Note that by default IPV6 binds to both protocols, it is must be disabled
    // if both protocols used at the same time (used in CI)
    setsockopt(listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
#endif

    ESP_LOGD(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {

        ESP_LOGD(TAG, "Socket listening...");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
#ifdef ENABLED_IPV6
        else if (source_addr.ss_family == PF_INET6)
        {
            inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
#endif
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        recive_data(sock);
        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

static void recive_data(const int sock)
{
    int len = 0;
    // div * leds * rgb
    ESP_LOGD(TAG, "Reciving data, allocate a rx_buffer");
    uint8_t *rx_buffer = (uint8_t *)malloc(320 * 50 * 3);
    size_t rx_buffer_size = 320 * 50 * 3;
    init_temp_frame_buffer();
    ESP_LOGD(TAG, "Allocate rx_buffer success");

    // Currently for debug
    int recv_count = 0;          // 接受了多少次,
    size_t recv_bytes_count = 0; // 总共接收了多少字节

    do
    {
        ESP_LOGD(TAG, "Start reciving data for %d times", recv_count);
        len = recv(sock, rx_buffer, rx_buffer_size, 0);
        recv_bytes_count += len;
        recv_count++;
        if (len < 0)
        {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d, Len = %d", errno, len);
        }
        else if (len == 0)
        {
            ESP_LOGW(TAG, "Connection closed");
        }
        else
        {
            ESP_LOGD(TAG, "Success recv %d bytes of data", len);
            // 正常接收到了数据
            // 将数据加入RTOS队列

            /*
             * 这里定义一下两边传输图片用的协议:
             * 总的来说, 一次传输50个rgb值结构体, 每个rgb值将包含3个uing8_t的r, g, b, 作为一个数据包
             * 每320个数据包为一帧图像
             */

            // 收到数据后, 需要将数据构造成img_frame_buffer_t, 然后push进队列里

            for (size_t buffer_index = 0; buffer_index < len; buffer_index++)
            {
                fill_temp_frame_buffer(rx_buffer[buffer_index]);
            }
            img_frame_buffer_t frame = update_temp_frame_buffer();
            if (frame != NULL)
            {
                ESP_LOGD(TAG, "Setted data to frames. push to queue");
                push_frame_to_queue(&frame);
                ESP_LOGD(TAG, "Push frame to queue finished");
            }

            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation.
            int to_write = len;
            while (to_write > 0) {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }
        }

    } while (len > 0);
}