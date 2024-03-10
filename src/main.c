#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include <SPI.h>
#include <AD7091R.h>

#define GPIO_TEST 7

void ad7091r_scan()
{
    while(true)
    {
        // // origin ad7091r task
        // uint16_t adc_value = ad7091r_read(ad7091r_spi_handle);
        // float volt = ad7091r_convert_to_volt(adc_value, VREF);
        // ESP_LOGI("ad7091r_scan", "value: %.3f (V)", volt);

        // gpio test, (we got 200 ns on )
        gpio_set_level(GPIO_TEST, 0);
        gpio_set_level(GPIO_TEST, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

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

    xTaskCreate(&ad7091r_scan, 
                "ad7091r_scan",
                2048,
                NULL,
                5,
                NULL);
}