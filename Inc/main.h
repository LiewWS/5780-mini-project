#pragma once
#include <stm32f0xx_hal.h>
#include <stdint.h>

void SystemClock_Config(void);

void HAL_RCC_GPIOC_CLK_ENABLE(void);
void HAL_RCC_GPIOA_CLK_ENABLE(void);
void HAL_RCC_GPIOB_CLK_ENABLE(void);
void HAL_RCC_SYSCFG_CLK_ENABLE(void);
void HAL_RCC_TIM2_CLK_ENABLE(void);
void HAL_RCC_TIM3_CLK_ENABLE(void);
void HAL_RCC_USART3_CLK_ENABLE(void);
void HAL_RCC_I2C2_CLK_ENABLE(void);
void HAL_RCC_ADC1_CLK_ENABLE(void);
void HAL_RCC_USART1_CLK_ENABLE(void);

void HAL_config_EXTI(uint32_t line_num, uint32_t control);

int bt_conf_main(void);
int bt_magnetic_enc_main(void);

int bt_magnetic_enc_serial_main(void);

void send_main(void);

#define DEBUG
