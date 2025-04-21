#ifndef BLUETOOTH_BUF_H
#define BLUETOOTH_BUF_H

#include <stdint.h>

#define SENDER
// Reserve byte value 0xff to separate angles
// Max value = 0x7FFE
// Small error low byte is 0xFF by changing it to 0xFE

#define SEP_BYTE 0xFF

#define BUF_SIZE 128
extern uint8_t recv_buf[BUF_SIZE];
extern uint8_t buf_head;
extern uint8_t buf_tail;

void inc_idx(uint8_t *idx);

#endif
