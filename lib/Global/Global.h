#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <sys/socket.h>

#define BUFFER_SIZE 2000

extern uint16_t ping_buffer[BUFFER_SIZE];
extern uint16_t pong_buffer[BUFFER_SIZE];
extern float wifi_buffer[BUFFER_SIZE];
extern uint16_t* active_buffer;
extern uint16_t* processing_buffer;
extern uint16_t buffer_index;
extern bool wifi_is_not_connect;

// init in main.c
extern SemaphoreHandle_t ad7091r_parse_semaphore;
extern SemaphoreHandle_t wifi_send_semaphore;
extern SemaphoreHandle_t timer_semaphore;

// wifi
extern int tcp_socket;
extern int udp_socket;
extern struct sockaddr_in server_addr;
#define TARGET_TCP_PORT 11520
#define TARGET_TCP_IP   "192.168.0.195"