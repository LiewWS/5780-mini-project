#include "magnetic_encoder.h"
#include "hal_usart.h"
#include "main.h"

#define SENDER
// Reserve byte value 0xff to separate angles
// Max value = 0x7FFE
// Small error low byte is 0xFF by changing it to 0xFE

#define SEP_BYTE 0xFF

void send_main(void);
void recv_main(void);

#define BUF_SIZE 128
uint8_t recv_buf[BUF_SIZE];
uint8_t buf_head;
uint8_t buf_tail;

void inc_idx(uint8_t *idx)
{
    uint8_t cur_idx = *idx;
    if (cur_idx >= BUF_SIZE)
    {
        *idx = 0;
    }
    else
    {
        *idx = cur_idx + 1;
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

static void send_main(void)
{
    // Initialize USART for bluetooth (from checkpoint 1)
    HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_RCC_USART1_CLK_ENABLE();

    configure_TTL(USART1, HAL_RCC_GetHCLKFreq() / 9600);
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
        status = read_i2c(0x36);
        if (status & 0x20)
        {
            writtenData[0] = 0x0C;
            write_i2c(writtenData, MAG_ADDR, 1);
            angle = read_i2c(MAG_ADDR) << 8;

            writtenData[0] = 0x0D;
            write_i2c(writtenData, MAG_ADDR, 1);
            angle |= read_i2c(MAG_ADDR);
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

            send_angle(USART1, angle);
        }
    }
}


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
    configure_TTL(USART3, HAL_RCC_GetHCLKFreq()/115200);
    // USART3 TX Pin (connect to RX of serial converter)
    GPIO_InitTypeDef init_pb10 = {GPIO_PIN_10, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 4};
    HAL_GPIO_Init(GPIOB, &init_pb10);
    // USART3 RX Pin (connect to TX of serial converter)
    GPIO_InitTypeDef init_pb11 = {GPIO_PIN_11, GPIO_MODE_AF_PP, GPIO_SPEED_FREQ_LOW, GPIO_NOPULL, 4};
    HAL_GPIO_Init(GPIOB, &init_pb11);

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

void USART1_IRQHandler(void)
{
    recv_buf[buf_head] = (char)USART1->RDR;
    inc_idx(&buf_head);
}


