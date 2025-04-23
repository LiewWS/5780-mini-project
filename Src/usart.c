#include "hal_usart.h"


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
    snprintf(str, sizeof(str), "%ld\r\n", numb);
    USART_send_string(USARTx, str);
}

// set up USART for debugging
void setup_USART(void)
{

    // set up GPIO pins for USART
    // pin 10 = Tx pin 11 = Rx
    GPIO_InitTypeDef initStrTXRX = {GPIO_PIN_10 | GPIO_PIN_11,
                                    GPIO_MODE_AF_PP,
                                    GPIO_SPEED_FREQ_LOW,
                                    GPIO_NOPULL,
                                    GPIO_AF1_USART3};

    HAL_GPIO_Init(GPIOC, &initStrTXRX);

    configure_TTL(USART3, HAL_RCC_GetHCLKFreq() / 115200);
}


// helper function to print what is in registers to help debugging
void printR(char *comment, uint32_t reg)
{
    char r[50];
    sprintf(r, "%s: 0x%08lX", comment, reg);
    USART_send_string(USART3, r);
}

// helper function to print actaul decimal numbers to help debugging
void printD(char *comment, int32_t numb)
{
    char str[20];
    snprintf(str, sizeof(char)*20, "%ld", numb); // Convert hex to a decimal string
    char str2[40];
    snprintf(str2, sizeof(char) * 40, "%s %s", comment, str);
    USART_send_string(USART3, str2);
}