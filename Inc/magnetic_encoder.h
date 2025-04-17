#include <stm32f0xx_hal.h>
#include <assert.h>
#include <stdio.h>
#include <hal_usart.h>
#include <stdlib.h>

#define MAG_ADDR 0x36

int magnetic_encoder_main(void);
void init_i2c();
uint8_t write_i2c(uint8_t* sent_dat, uint8_t sent_addr, uint8_t num_bytes);
uint8_t read_i2c(uint8_t addr);

