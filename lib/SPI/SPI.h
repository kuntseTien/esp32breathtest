#ifndef LIB_SPI_H_
#define LIB_SPI_H_

#include <driver/spi_master.h>

#include <Global.h>

/* export variable */
extern spi_device_handle_t ad7091r_spi_handle;

void spi_init(spi_device_handle_t* spi);
uint16_t spi_read(uint8_t operation_mode, uint8_t len, spi_device_handle_t spi);
void spi_read_2_byte_to_buf(uint8_t operation_mode, spi_device_handle_t handle);

#endif