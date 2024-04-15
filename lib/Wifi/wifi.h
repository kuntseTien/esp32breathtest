#ifndef LIB_WIFI_H_
#define LIB_WIFI_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// 定义WiFi SSID和密码
#define WIFI_SSID "602lab"
#define WIFI_PASS "602mems206"

// 初始化WiFi
void wifi_init(void);

// WiFi连接任务
void wifi_connect_task(void *pvParameters);

#endif