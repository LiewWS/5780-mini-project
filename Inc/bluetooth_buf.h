#ifndef BLUETOOTH_BUF_H
#define BLUETOOTH_BUF_H

#define SENDER
// Reserve byte value 0xff to separate angles
// Max value = 0x7FFE
// Small error low byte is 0xFF by changing it to 0xFE

#define SEP_BYTE 0xFF

#define BUF_SIZE 128
uint8_t recv_buf[BUF_SIZE];
uint8_t buf_head;
uint8_t buf_tail;

void USART1_IRQHandler(void)
{
    recv_buf[buf_head] = (char)USART1->RDR;
    inc_idx(&buf_head);
}

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

#endif
