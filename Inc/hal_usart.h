#ifndef HAL_USART_H
#define HAL_USART_H

#include <stdint.h>
#include <stm32f0xx_hal.h>

void configure_TTL(USART_TypeDef* USARTx, uint32_t brr_val);
void configure_TTL_RXint(USART_TypeDef* USARTx, uint32_t brr_val);
void USART_send_byte(USART_TypeDef* USARTx, char data);
void USART_send_string(USART_TypeDef* USARTx, char* data);
char USART_recv_byte(USART_TypeDef* USARTx);

void send_angle(USART_TypeDef* USARTx, uint16_t angle);
uint16_t read_angle(uint16_t old_angle);

#endif

