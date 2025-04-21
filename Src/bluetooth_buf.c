#include <stdint.h>
#include <stm32f0xx_hal.h>

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

void USART1_IRQHandler(void)
{
    recv_buf[buf_head] = (char)USART1->RDR;
    inc_idx(&buf_head);
}

