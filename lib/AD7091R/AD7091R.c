#include "driver/gpio.h"

#include <AD7091R.h>
#include <SPI.h>
#include <Timer.h>

/* private functions declaration */
static void ad7091r_software_reset(spi_device_handle_t spi);

/* public functions */

void ad7091r_init(spi_device_handle_t* spi)
{
    /* SPI init */
    spi_init(spi);

    /* pin setting */
    gpio_config_t io_conf;

    // 禁用中斷
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // 設置為輸出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 設置要操作的腳位
    io_conf.pin_bit_mask = (1ULL << AD7091R_CONVST);
    // 禁用上拉電阻
    io_conf.pull_up_en = 0;
    // 禁用下拉電阻
    io_conf.pull_down_en = 0;
    // 配置GPIO腳位
    gpio_config(&io_conf);

    ad7091r_software_reset(*spi);
}

/* private functions definition */
static void ad7091r_software_reset(spi_device_handle_t spi)
{
    // set CONVST low
    gpio_set_level(AD7091R_CONVST, 0);
    // set CONVST high (estimated ~ 200 ns)
    gpio_set_level(AD7091R_CONVST, 1);

    spi_read(OPERATION_MODE, 1, spi);
}

/* public function definition */
uint16_t ad7091r_read(spi_device_handle_t spi)
{
    uint16_t spi_read_result;

    // perform a delay?

    spi_read_result = spi_read(OPERATION_MODE, 2, spi);

    if (OPERATION_MODE == POWER_DOWN_MODE)
    {
        ad7091r_power_up();
    }

    return spi_read_result;
}

float ad7091r_convert_to_volt(uint16_t adc_value, float vref)
{
    float result;

    if (vref == 0.0f)
    {
        vref = 2.5f;
    }

    result = vref * (float)adc_value / ADC_RESOLUTION;
    return result;
}

void ad7091r_power_up()
{
    gpio_set_level(AD7091R_CONVST, 1);
    // sleep for 50 ms (to provent freertos watchdog bites!)
    timer_sleep(50);
}