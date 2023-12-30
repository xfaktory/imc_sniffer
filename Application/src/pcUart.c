/*
 * pcUart.c
 *
 *  Created on: 20 июл. 2023 г.
 *      Author: User
 */

#include "pcUart.h"
#include "usartDriver.h"
#include "protocol.h"
#include "ffDecoder.h"
#include "ffEncoder.h"

static bool pcUart_initState = false;
//static bool pcUart_USARTTransmit_f = false;
static ffDecoder_context pcUart_ffDecoderContext = {0};

static void pcUart_usartCallbackByte(uint8_t data);
static void pcUart_ffDecoderPayload(ffDecoder_callbackStruct *context);

void pcUart_init(){
	ffDecoder_Init(&pcUart_ffDecoderContext, pcUart_ffDecoderPayload);

	usartDriver_pushReadBuff(pcUart_usartCallbackByte);
	pcUart_initState = true;
}

void pcUart_deinit(){
	ffDecoder_DeInit(&pcUart_ffDecoderContext);
	pcUart_initState = false;
}

static void pcUart_usartCallbackByte(uint8_t data){
	if(!pcUart_initState){
		return;
	}
	ffDecoder_pushByte(&pcUart_ffDecoderContext, data);
}

static void pcUart_usartCallbackTx(){
//	pcUart_USARTTransmit_f = false;
}

static void pcUart_ffDecoderPayload(ffDecoder_callbackStruct *context){
	if(!pcUart_initState){
		return;
	}
	uint8_t i = 0;
	uint8_t buff[255] = {0};

	if(context->len < 1){
		return;
	}
	for(i = 0; i < context->len; i++){
		buff[i] = CIRCULAR_BUF_EXT(i, context->packetPayload, context->bufferSize, context->buffer);
	}
	if(!protocol_pushComm(buff, i)){
		//ERROR
	}
}

void pcUart_exec(){
	if(!pcUart_initState){
		return;
	}
	ffDecoder_process(&pcUart_ffDecoderContext);
}

bool pcUart_pushSendBuff(uint8_t *buff, uint8_t len){	//отправить данные на ПК
	uint8_t buffSend[255] = {0};

	if((!buff) || (len == 0) || (len > 254)){
		return false;
	}
	len = ffEncoder_encode(buff, len, buffSend);
	if(!usartDriver_pushWriteBuff(buffSend, len, pcUart_usartCallbackTx)){
		return false;
	}
	return true;
}
