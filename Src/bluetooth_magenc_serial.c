#include "magnetic_encoder.h"
#include "hal_usart.h"
#include "main.h"
#include "bluetooth_buf.h"

void setup_USART(void);

static void recv_main(void)
{
    // Initialize LEDs
    HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef init_pc = {GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
                                GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &init_pc);
    // Orange LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);
    // Green LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
    // Red LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
    // Blue LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

    // Initialize USART for bluetooth (from checkpoint 1)
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();
    configure_TTL_RXint(USART1, HAL_RCC_GetHCLKFreq() / 9600);
    // USART1 TX Pin (connect to RX of bluetooth)
    GPIO_InitTypeDef init_pa9 = {GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa9);
    // USART1 RX Pin (connect to TX of bluetooth)
    GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa10);
    // Set up NVIC
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 1);

    // Initialize USART for serial
    HAL_RCC_USART3_CLK_ENABLE();
    setup_USART();

    // Initialize receive buffer
    buf_head = 0;
    buf_tail = 0;

    uint16_t angle = 0;

    while (1)
    {
        // Main loop
        angle = read_angle(angle);
        USART_printD(USART3, angle);

        if (angle < 1000)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 0);

        if ((angle > 1000) && (angle < 2000))
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 1);
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);
        if ((angle > 2000) && (angle < 3000))
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 1);
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, 0);

        if (angle > 3000)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 1);
        }
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 0);
    }
}

int bt_magnetic_enc_serial_main(void)
{
#if defined(SENDER)
    send_main();
#else
    recv_main();
#endif
    return 1;
}



