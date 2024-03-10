#include "SPI.h"
#include <AD7091R.h>

#include <driver/gpio.h>

// global handle
spi_device_handle_t ad7091r_spi_handle;

void spi_init(spi_device_handle_t* spi)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = AD7091R_SDO, // MISO引脚号
        .mosi_io_num = -1, // MOSI引脚号
        .sclk_io_num = AD7091R_SCLK,  // 时钟引脚号
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096, // 最大传输大小
    };

    spi_device_interface_config_t devcfg = {
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz = 5*1000*1000, // 时钟速度5MHz
        .mode = 0,                       // SPI模式0
        .spics_io_num = AD7091R_CS,      // 片选引脚号
        .queue_size = 7,                 // 事务队列大小
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, spi));
}

uint16_t spi_read(uint8_t operation_mode, uint8_t len, spi_device_handle_t handle)
{
    uint16_t low_byte;
    uint16_t high_byte;
    uint16_t result;

    spi_transaction_t t = {0};
    uint8_t rx_buffer[2] = {0};

    t.length = (len == 1U) ? 8 : 16 ;
    t.rxlength = (len == 1U) ? 8 : 16 ; // 1 byte or two byte
    t.tx_buffer = NULL;
    t.rx_buffer = rx_buffer;

    // flush fifo?

    gpio_set_level(AD7091R_CONVST, 0);
    if (operation_mode == NORMAL_MODE)
    {
        gpio_set_level(AD7091R_CONVST, 1);
    }

    spi_device_transmit(handle, &t);

    // if power up mode
    if (operation_mode == POWER_DOWN_MODE)
    {
        gpio_set_level(AD7091R_CONVST, 1);
    }

    high_byte = rx_buffer[0];
    low_byte = rx_buffer[1];
    result = (len == 1U) ? high_byte : (uint16_t)((high_byte << 8 | low_byte) >> 4) ;
    
    return result;
}