#include <stm32f0xx_hal.h>
#include <assert.h>
#include <stdio.h>
#include "main.h"
#include "hal_usart.h"

int test_main(void) {
    HAL_Init(); //resest of all peripherals, init the flash and systick
    SystemClock_Config(); //configure the system clock


    HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_RCC_USART3_CLK_ENABLE();

    //set up GPIO pins for USART
    //pin 10 = Tx pin 11 = Rx
    GPIO_InitTypeDef initStrTXRX = {GPIO_PIN_10 | GPIO_PIN_11, 
        GPIO_MODE_AF_PP,  
        GPIO_SPEED_FREQ_LOW, 
        GPIO_NOPULL,
        GPIO_AF1_USART3};


    HAL_GPIO_Init(GPIOC, &initStrTXRX);

    configure_TTL(USART3, HAL_RCC_GetHCLKFreq()/115200);

    while(1)
    {
        USART_send_string(USART3, "Hello");
    }
}

