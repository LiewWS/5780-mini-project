#include <assert.h>
#include "main.h"
#include "hal_usart.h"

void control_LED(char led_sel);

#define TEST_UART_SERIAL 0
#define TEST_BT_CONNECT  1

int bt_conf_main(void) {
    HAL_Init();
    SystemClock_Config();

    // Enable clocks
    HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_RCC_GPIOA_CLK_ENABLE();

    HAL_RCC_USART1_CLK_ENABLE();
    HAL_RCC_USART3_CLK_ENABLE();

    // USART1 (to bluetooth)
#if (TEST_BT_CONNECT)
    configure_TTL(USART1, HAL_RCC_GetHCLKFreq()/9600);
#endif

    // USART3 (to terminal)
#if (TEST_UART_SERIAL == 1)
    configure_TTL(USART3, HAL_RCC_GetHCLKFreq()/115200);
#elif (TEST_BT_CONNECT == 1)
    configure_TTL_RXint(USART3, HAL_RCC_GetHCLKFreq()/9600);
#endif

    // USART3 TX Pin (connect to RX of serial converter)
    GPIO_InitTypeDef init_pb10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 4};
    HAL_GPIO_Init(GPIOB, &init_pb10);

    // USART3 RX Pin (connect to TX of serial converter)
    GPIO_InitTypeDef init_pb11 = {GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 4};
    HAL_GPIO_Init(GPIOB, &init_pb11);

    // USART1 TX Pin (connect to RX of bluetooth)
    GPIO_InitTypeDef init_pa9 = {GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa9);
    // USART1 RX Pin (connect to TX of bluetooth)
    GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa10);

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

#if (TEST_UART_SERIAL == 1)
    char led_select = 0;
#endif
    // Set up NVIC
    NVIC_EnableIRQ(USART3_4_IRQn);
    NVIC_SetPriority(USART3_4_IRQn, 1);

    while (1) {
#if (TEST_UART_SERIAL == 1)
        led_select = USART_recv_byte(USART3);
        control_LED(led_select);
#elif (TEST_BT_CONNECT == 1)
        HAL_Delay(100);
        USART_send_byte(USART1, 'r');
#endif
    } 

    return 1;
}

void control_LED(char led_sel)
{
    // char* error_msg = "ERROR: Invalid character.\r\n";
    switch (led_sel) {
        case 'r':
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_6);
            break;
        case 'b':
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
            break;
        case 'g':
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9);
            break;
        case 'o':
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
            break;
        default:
            // Send error
            // USART_send_string(USART3, error_msg);
            break;
    }
}

/* void USART3_4_IRQHandler(void)
{
  char c = (char) USART3->RDR;
#if (TEST_BT_CONNECT == 1)
  control_LED(c);
#endif
} */

