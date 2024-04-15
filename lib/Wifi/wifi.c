#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <sys/socket.h>
#include "lwip/err.h"
#include "lwip/sockets.h"
#include <string.h>

#include "wifi.h"
#include <Global.h>

#define PORT 8080

static const char *TAG = "WiFiConnect";
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int FAIL_BIT = BIT1;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(wifi_event_group, FAIL_BIT);
        esp_wifi_connect();
    }
}

void wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void setup_tcp_server(void) {
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_socket < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int err = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(server_socket);
        return;
    }

    err = listen(server_socket, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(server_socket);
        return;
    }

    ESP_LOGI(TAG, "TCP server listening on port %d", PORT);
}

void wifi_connect_task(void *pvParameters) {
    wifi_init();
    
    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                               CONNECTED_BIT | FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               portMAX_DELAY);

        if (bits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected");
            setup_tcp_server();  // Set up TCP server after WiFi is connected
            ESP_LOGI(TAG, "TCP Server Set Up");
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        } else if (bits & FAIL_BIT) {
            ESP_LOGI(TAG, "Failed to connect to WiFi. Retrying...");
            xEventGroupClearBits(wifi_event_group, FAIL_BIT);
        } else {
            ESP_LOGE(TAG, "Unexpected event");
        }
    }
}
