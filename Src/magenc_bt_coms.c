#include "magenc_bt_coms.h"

uint8_t read_angle(uint16_t* angle)
{
    uint16_t new_angle = *angle;
    if (buf_head > buf_tail)
    {
        if (recv_buf[buf_tail] == SEP_BYTE)
        {
            // Step over separator byte and return old angle
            inc_idx(&buf_tail);
            return 0;
        }
        else
        {
            new_angle = recv_buf[buf_tail] << 8;
            inc_idx(&buf_tail);
            if (buf_head <= buf_tail) {
                return 0;
            }
            if (recv_buf[buf_tail] == SEP_BYTE)
            {
                // Step over separator byte and return old angle
                inc_idx(&buf_tail);
                return 0;
            }
            else
            {
                // Here we got two consecutive data bytes
                *angle = new_angle + recv_buf[buf_tail];
                inc_idx(&buf_tail);
            }
            return 1;
        }
    }
    return 0;
}

void send_main(void)
{
    // Initialize USART for bluetooth (from checkpoint 1)
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();

    configure_TTL(USART1, HAL_RCC_GetHCLKFreq() / 115200);
    // USART1 TX Pin (connect to RX of bluetooth)
    GPIO_InitTypeDef init_pa9 = {GPIO_PIN_9 | GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa9);
    // USART1 RX Pin (connect to TX of bluetooth)
    // GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    // HAL_GPIO_Init(GPIOA, &init_pa10);

    // Initialize I2C for magnetic encoder (from checkpoint 2)
    init_i2c();
    uint8_t writtenData[1] = {0x0B};
    write_i2c(writtenData, MAG_ADDR, 1);
    uint8_t status = read_i2c(MAG_ADDR);
    uint16_t angle = 0;
    while (1)
    {
        writtenData[0] = 0x0B;
        write_i2c(writtenData, MAG_ADDR, 1);
        //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, 1);
        status = read_i2c(0x36);
        if (status & 0x20)
        {
            writtenData[0] = 0x0C;
            write_i2c(writtenData, MAG_ADDR, 1);
            angle = read_i2c(MAG_ADDR) << 8;

            writtenData[0] = 0x0D;
            write_i2c(writtenData, MAG_ADDR, 1);
            angle |= read_i2c(MAG_ADDR);
//#if defined()
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
//#endif

            send_angle(USART1, angle);
        }
    }
}

void send_angle(USART_TypeDef *USARTx, uint16_t angle)
{
    uint8_t b0 = (uint8_t)((angle >> 8) & 0x00FF);
    if (b0 == SEP_BYTE)
    {
        // SEP_BYTE = 0xFF
        b0 = 0x7F;
    }
    USART_send_byte(USARTx, b0);

    uint8_t b1 = (uint8_t)(angle & 0x00FF);
    if (b1 == SEP_BYTE)
    {
        // SEP_BYTE = 0xFF
        b1 = 0xFE;
    }
    USART_send_byte(USARTx, b1);

    // Send separator byte
    USART_send_byte(USARTx, SEP_BYTE);
}


void recv_main(void)
{
    uint16_t LED_GPIO_PINS = (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_6 | GPIO_PIN_7);
    // Initialize LEDs
    HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef init_pc = {LED_GPIO_PINS,
                                GPIO_MODE_OUTPUT_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL};
    HAL_GPIO_Init(GPIOC, &init_pc);
    // Orange LED
    HAL_GPIO_WritePin(GPIOC, LED_GPIO_PINS, GPIO_PIN_RESET);
    /*
    // Green LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
    // Red LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
    // Blue LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
    */

    // Initialize USART for bluetooth (from checkpoint 1)
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();
    configure_TTL_RXint(USART1, HAL_RCC_GetHCLKFreq() / 9600);
    // USART1 TX Pin PA9 (connect to RX of bluetooth)

    GPIO_InitTypeDef init_pa_9_10 = {GPIO_PIN_9 | GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    HAL_GPIO_Init(GPIOA, &init_pa_9_10);
    // USART1 RX Pin PA10 (connect to TX of bluetooth)

    // GPIO_InitTypeDef init_pa10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 1};
    // HAL_GPIO_Init(GPIOA, &init_pa10);
    //  Set up NVIC
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 1);

#ifdef BTENCSER
    // Initialize USART for serial
    HAL_RCC_USART3_CLK_ENABLE();
    setup_USART();
#endif
    // Initialize receive buffer
    buf_head = 0;
    buf_tail = 0;

    uint16_t angle = 0;

    while (1)
    {
        // Main loop
        read_angle(&angle);

#ifdef BTENCSER
        USART_printD(USART3, angle);
#endif

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

        if (angle >= 3000)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 1);
        }
        else
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, 0);
        }
    }
}

