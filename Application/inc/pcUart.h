/*
 * pcUart.h
 *
 *  Created on: 20 июл. 2023 г.
 *      Author: User
 */

#ifndef INC_PCUART_H_
#define INC_PCUART_H_


#include "main.h"

void pcUart_init();
void pcUart_deinit();
void pcUart_exec();

bool pcUart_pushSendBuff(uint8_t *buff, uint8_t len);

#endif /* INC_PCUART_H_ */
