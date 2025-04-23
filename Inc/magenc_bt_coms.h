#ifndef MAGENC_BT_COMS_H
#define MAGENC_BT_COMS_H

#include "magnetic_encoder.h"
#include "hal_usart.h"
#include "main.h"
#include "bluetooth_buf.h"

/// @brief sends a 16 bit angle over USART
/// @param USARTx the USART port
/// @param angle the unsigned integer angle
void send_angle(USART_TypeDef* USARTx, uint16_t angle);
/// @brief 
/// @param old_angle 
/// @return 
uint8_t read_angle(uint16_t* old_angle);
/// @brief 
/// @param  
void send_main(void);
/// @brief 
/// @param  
void recv_main(void);

#endif
