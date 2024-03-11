#ifndef LIB_AD7091R_H_
#define LIB_AD7091R_H_

#include <stdint.h>
#include <driver/spi_master.h>

#define NORMAL_MODE 1
#define POWER_DOWN_MODE 2
#define ADC_RESOLUTION (1 << 12)

/* User defined */
#define OPERATION_MODE NORMAL_MODE
#define VREF 2.5f

#define AD7091R_CS     10
#define AD7091R_SCLK   12
#define AD7091R_SDO    13
#define AD7091R_CONVST 16

#define SPI_CLOCK_SPEED 20 // M

/* Functions */
void ad7091r_init(spi_device_handle_t* spi);
uint16_t ad7091r_read(spi_device_handle_t spi);
void ad7091r_read_to_buf(spi_device_handle_t spi);
void ad7091r_power_up();
float ad7091r_convert_to_volt(uint16_t adc_value, float v_ref);
void ad7091r_convert_to_volt_buf(uint16_t* raw_buf, float* target_buf, uint16_t len, float vref);

#endif