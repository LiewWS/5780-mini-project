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
/// @brief read the angle from the encoder
/// @param old_angle the old angle of the magnet, which will be overwritten with the new angle 
/// @return return 1 if the angle has changed, 0 o.w.
uint8_t read_angle(uint16_t* old_angle);
/// @brief Sends the data through UART1 to the bluetooth module
/// @param 
void send_main(void);
/// @brief Recieves data from the bluetooth module through UART1
/// @param  
void recv_main(void);

#endif
