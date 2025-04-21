#include "hal_usart.h"
#include <stdio.h>

void configure_TTL(USART_TypeDef* USARTx, uint32_t brr_val) 
{
    // Set baud rate
    USARTx->BRR |= brr_val;
    // Enable RX and TX
    USARTx->CR1 |= (3 << 2);
    // Enable USART
    USARTx->CR1 |= 1;
}

void configure_TTL_RXint(USART_TypeDef* USARTx, uint32_t brr_val) 
{
    USARTx->CR1 |= (1 << 5);
    configure_TTL(USARTx, brr_val);
}

void USART_send_byte(USART_TypeDef* USARTx, char data)
{
    // Wait for data to be transferred from TDR to shift register
    while ((USARTx->ISR & (1 << 7)) == 0);
    // Place new data in TDR register
    USARTx->TDR = data;
}

void USART_send_string(USART_TypeDef* USARTx, char* data)
{
    while (*data != '\0') {
        USART_send_byte(USARTx, *data);
        ++data;
    }
}

char USART_recv_byte(USART_TypeDef* USARTx)
{
    // Wait for received data to be ready for reading
    while((USARTx->ISR & (1 << 5)) == 0);
    // Read received data
    char val = (char) USARTx->RDR;
    return val;
}

void USART_printD(USART_TypeDef* USARTx, int32_t numb)
{
    char str[10];
    snprintf(str, sizeof(str), "%d", numb);
    USART_send_string(USARTx, str);
}
