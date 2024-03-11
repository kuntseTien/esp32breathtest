#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"

#include "esp_attr.h"
#include "esp_log.h"
#include <sys/socket.h>

#include <SPI.h>
#include <AD7091R.h>
#include <wifi.h>
#include <Self/miniz.h>
#include <string.h>

#define GPIO_TEST 7

#define TIMER_INTERVAL_SEC 0.0005 // 示例：2kHz的频率对应的间隔为0.0005秒

#define CHUNK_SIZE 1024

static gptimer_handle_t timer_handle = NULL;

// timer callback
void ad7091r_scan_to_buf(void *para)
{
    // check buffer index is valid
    if (buffer_index >= BUFFER_SIZE)
    {
        // change to other buffer
        buffer_index = 0;
        if (active_buffer == ping_buffer)
        {
            active_buffer = pong_buffer;
            processing_buffer = ping_buffer;
        }
        else
        {
            active_buffer = ping_buffer;
            processing_buffer = pong_buffer;
        }

        // perform parse signal, assume timer call back won't be interrupted
        xSemaphoreGive(ad7091r_parse_semaphore);

        ESP_LOGI("ad7091r_scan_to_buf", "perform parse procedure");
    }

    ad7091r_read_to_buf(ad7091r_spi_handle);

    // maintain index
    ++buffer_index;
}

void ad7091r_parse(void *para)
{
    while(1)
    {
        if (xSemaphoreTake(ad7091r_parse_semaphore, portMAX_DELAY) == pdTRUE)
        {
            // parse processing data buffer to wifi_tx_buffer
            ad7091r_convert_to_volt_buf(processing_buffer, wifi_buffer, BUFFER_SIZE, VREF);

            // send transmit semaphore
            xSemaphoreGive(wifi_send_semaphore);
        }
    }
}

static bool timer_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_awoken = pdFALSE;

    // 通知等待的任务
    xSemaphoreGiveFromISR(timer_semaphore, &high_task_awoken);

    // 返回是否需要在中断结束时进行上下文切换
    return high_task_awoken == pdTRUE;
}

void ad7091r_task(void *pvParameters) {
    while (1) {
        // 等待信号量
        if (xSemaphoreTake(timer_semaphore, portMAX_DELAY) == pdPASS) {
            // 执行周期性任务
            ad7091r_scan_to_buf(NULL);

            // 如果需要，在此处翻转GPIO
            static uint8_t level = 0;
            level = 1 - level;
            gpio_set_level(GPIO_TEST, level);
        }
    }
}

void initialize_timer(void) {
    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // 使用默认时钟源(APB_CLK)
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1e6, // 配置定时器的分辨率为1MHz
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&config, &timer_handle));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = TIMER_INTERVAL_SEC * 1e6, // 定时器间隔，单位为ticks
        .flags.auto_reload_on_alarm = true, // 设置为true以实现周期性定时器
    };
    
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_handle, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_callback, // 注册回调函数
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer_handle, &cbs, NULL));

    // 启动定时器
    // not good but it's a workaround
    gptimer_enable(timer_handle);
    gptimer_start(timer_handle);

    // perform measurement
    xTaskCreate(&ad7091r_task,
                "ad7091r_task",
                2048,
                NULL,
                10,
                NULL);
}

// constant length
void compress_and_transmit(void *para)
{
    while(1)
    {
        if (xSemaphoreTake(wifi_send_semaphore, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI("ad7091r_scan_to_buf", "perform compression procedure");

            mz_stream stream;
            memset(&stream, 0, sizeof(stream));
            mz_deflateInit(&stream, MZ_DEFAULT_COMPRESSION);

            const uint16_t buf_len = BUFFER_SIZE * sizeof(float);
            uint8_t* buf = (uint8_t*) wifi_buffer;

            // 假设我们逐块处理数据
            for (size_t i = 0; i < buf_len * sizeof(float); i += CHUNK_SIZE) {
                size_t chunk_size = (buf_len * sizeof(float) - i < CHUNK_SIZE) ? (buf_len * sizeof(float) - i) : CHUNK_SIZE;
                stream.next_in = (const unsigned char*)buf + i;
                stream.avail_in = chunk_size;

                uint8_t out[CHUNK_SIZE]; // 假设输出不会超过输入
                stream.next_out = out;
                stream.avail_out = CHUNK_SIZE;

                // 进行压缩
                int status = mz_deflate(&stream, MZ_NO_FLUSH);
                assert(status == MZ_OK);

                // 计算实际的输出大小
                size_t out_size = CHUNK_SIZE - stream.avail_out;

                // 通过UDP发送压缩后的数据
                sendto(udp_socket, out, out_size, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            }

            mz_deflateEnd(&stream);
        }
        
    }
}

// void ad7091r_scan(void *para)
// { 
//     uint16_t adc_value = ad7091r_read(ad7091r_spi_handle);
//     float volt = ad7091r_convert_to_volt(adc_value, VREF);
//     ESP_LOGI("ad7091r_scan", "value: %.3f (V)", volt);
// }

void app_main() 
{
    ESP_LOGI("app_main", "Initialization start...");
    ESP_LOGI("app_main", "Try to init AD7091R...");
    ad7091r_init(&ad7091r_spi_handle);
    ESP_LOGI("app_main", "Init process successed.");

    /* gpio test */
    gpio_config_t io_conf;

    // 禁用中斷
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // 設置為輸出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 設置要操作的腳位
    io_conf.pin_bit_mask = (1ULL << GPIO_TEST);
    // 禁用上拉電阻
    io_conf.pull_up_en = 0;
    // 禁用下拉電阻
    io_conf.pull_down_en = 0;
    // 配置GPIO腳位
    gpio_config(&io_conf);

    /* semaphore create */
    ad7091r_parse_semaphore = xSemaphoreCreateBinary();
    wifi_send_semaphore = xSemaphoreCreateBinary();
    timer_semaphore = xSemaphoreCreateBinary();

    /* wifi connection */
    xTaskCreate(&wifi_connect_task, 
            "wifi_connect_tassk",
            4096,
            NULL,
            5,
            NULL);

    while(wifi_is_not_connect)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    xTaskCreate(&ad7091r_parse, 
                "ad7091r_parse",
                2048,
                NULL,
                3,
                NULL);

    xTaskCreate(&compress_and_transmit,
                "compression_and_transmit",
                4096,
                NULL,
                7,
                NULL);

    /* initialize timer */
    initialize_timer();
                
}