#ifndef HAL_USART_H
#define HAL_USART_H

#include <stdint.h>
#include <stm32f0xx_hal.h>
#include <stdio.h>

void configure_TTL(USART_TypeDef* USARTx, uint32_t brr_val);
void configure_TTL_RXint(USART_TypeDef* USARTx, uint32_t brr_val);
void USART_send_byte(USART_TypeDef* USARTx, char data);
void USART_send_string(USART_TypeDef* USARTx, char* data);
char USART_recv_byte(USART_TypeDef* USARTx);

void USART_printD(USART_TypeDef* USARTx, int32_t numb);

void setup_USART(void);
void printR(char *comment, uint32_t reg);
void printD(char *comment, int32_t numb);

#endif

