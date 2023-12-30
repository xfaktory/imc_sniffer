/*
 * usartDriver.h
 *
 *  Created on: 20 июн. 2023 г.
 *      Author: User
 */

#ifndef INC_USARTDRIVER_H_
#define INC_USARTDRIVER_H_

#include "main.h"

void usartDriver_init();
void usartDriver_deinit();
void usartDriver_exec();

void usartDriver_TXEmpty_Callback(void);
void usartDriver_TransmitComplete_Callback(void);
void usartDriver_ErrorCallback(void);
void usartDriver_IRQRxHandler();

bool usartDriver_pushWriteBuff( uint8_t *buff, uint16_t len, void (*fnCallback)(void) );
bool usartDriver_pushReadBuff( void (*fnCallback)(uint8_t) );

#endif /* INC_USARTDRIVER_H_ */
