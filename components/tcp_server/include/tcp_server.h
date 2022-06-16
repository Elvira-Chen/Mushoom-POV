#pragma once

#include "lwip/sockets.h"

typedef struct
{
    int addr_family;
    void *param;
} tcp_server_task_param_t;

void tcp_server_task(void *pvParameters);