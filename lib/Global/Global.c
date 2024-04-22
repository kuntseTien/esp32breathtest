#include <Global.h>
#include <stdbool.h>

uint16_t ping_buffer[BUFFER_SIZE];
uint16_t pong_buffer[BUFFER_SIZE];
float wifi_buffer[BUFFER_SIZE];
uint16_t* active_buffer = ping_buffer;
uint16_t* processing_buffer = pong_buffer;
uint16_t buffer_index = 0;

SemaphoreHandle_t wifi_send_semaphore;
SemaphoreHandle_t ad7091r_parse_semaphore;
SemaphoreHandle_t timer_semaphore;

int tcp_socket;
struct sockaddr_in server_addr;
bool wifi_is_not_connect = true;