/*
 * usartDriver.c
 *
 *  Created on: 20 июн. 2023 г.
 *      Author: User
 */


#include "usartDriver.h"
#include "stddef.h"
#include "string.h"

#define USARTDRIVER_QUEUE_COUNT	10
#define USARTDRIVER_RX_BUFF_DMA_SIZE	255

typedef enum{
	QUEUE_STATE_NONE = (uint8_t)0,
	QUEUE_STATE_WRITE,
	QUEUE_STATE_READ,
}usartDriver_queueState_t;

typedef struct{
	uint8_t buff[255];
	uint16_t len;
	uint16_t count;
	usartDriver_queueState_t state;
	void (*fnCallback)(void);
}usartDriver_contextTX_t;

typedef struct{
	usartDriver_queueState_t state;
	void (*fnCallback)(uint8_t);
}usartDriver_contextRX_t;

static modulesState_t modulesState = MODULES_STATE_NO;
static usartDriver_contextRX_t usartDriver_RX;
static usartDriver_contextTX_t usartDriver_queue[USARTDRIVER_QUEUE_COUNT];
static uint8_t usartDriver_queueCount = 0;
static uint8_t usartDriver_curentTransactionNum;	//номер в очереди текущей транзакции
static bool usartDriver_transaction_f = false;
static uint8_t usartDriver_RxBuff_DMA[USARTDRIVER_RX_BUFF_DMA_SIZE] = {0};

static void usartDriver_clearQueueNum(uint8_t num);

static void usartDriver_sendUsartIT(){
    LL_LPUART_TransmitData8(LPUART1, usartDriver_queue[usartDriver_curentTransactionNum].buff[usartDriver_queue[usartDriver_curentTransactionNum].count++]);
    LL_LPUART_EnableIT_TXE(LPUART1);
}

void usartDriver_TXEmpty_Callback(void){
	if(usartDriver_queue[usartDriver_curentTransactionNum].count == (usartDriver_queue[usartDriver_curentTransactionNum].len - 1)){
		LL_LPUART_DisableIT_TXE(LPUART1);
		LL_LPUART_EnableIT_TC(LPUART1);
	}
	LL_LPUART_TransmitData8(LPUART1, usartDriver_queue[usartDriver_curentTransactionNum].buff[usartDriver_queue[usartDriver_curentTransactionNum].count++]);
}

void usartDriver_TransmitComplete_Callback(void){
	LL_LPUART_DisableIT_TC(LPUART1);
	if( usartDriver_queue[usartDriver_curentTransactionNum].fnCallback ){
		usartDriver_queue[usartDriver_curentTransactionNum].fnCallback();
	}
	usartDriver_clearQueueNum(usartDriver_curentTransactionNum);
	usartDriver_curentTransactionNum++;
	usartDriver_transaction_f = false;
}

void usartDriver_ErrorCallback(void){
	usartDriver_clearQueueNum(usartDriver_curentTransactionNum);
	usartDriver_curentTransactionNum++;
	usartDriver_transaction_f = false;
}

static void usartDriver_clearQueueNum(uint8_t num){
	usartDriver_queue[num].count = 0;
	usartDriver_queue[num].state = QUEUE_STATE_NONE;
	if(usartDriver_queueCount){
		usartDriver_queueCount--;
	}
}

void usartDriver_IRQRxHandler(){
	static uint32_t old_pos = 0;
	uint32_t pos;

    pos = USARTDRIVER_RX_BUFF_DMA_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3);

    if((usartDriver_RX.state == QUEUE_STATE_NONE) || (usartDriver_RX.fnCallback == NULL)){
    	old_pos = pos /* % USARTDRIVER_RX_BUFF_DMA_SIZE*/;
    	return;
    }
	while(old_pos != pos){
		usartDriver_RX.fnCallback(usartDriver_RxBuff_DMA[old_pos]);
		old_pos++;
		old_pos %= USARTDRIVER_RX_BUFF_DMA_SIZE;
	}
}

static void usartDriver_initQueue(){
	uint8_t i;

	for(i = 0; i < USARTDRIVER_QUEUE_COUNT; i++){
		usartDriver_queue[i].state = QUEUE_STATE_NONE;
	}
	usartDriver_RX.state = QUEUE_STATE_NONE;
}

static void usartDriver_initUsartDMA(){

	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t) & (LPUART1->RDR));
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)usartDriver_RxBuff_DMA);
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, USARTDRIVER_RX_BUFF_DMA_SIZE);

//	LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_3);
//	LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_3);
//	LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_3);

	LL_LPUART_EnableDMAReq_RX(LPUART1);
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
}

void usartDriver_init(){
	usartDriver_initQueue();

	usartDriver_initUsartDMA();
	LL_LPUART_EnableIT_IDLE(LPUART1);
	LL_LPUART_EnableIT_RXNE(LPUART1);
	LL_LPUART_Enable(LPUART1);

	modulesState = MODULES_STATE_INIT;
}

void usartDriver_deinit(){
	modulesState = MODULES_STATE_DEINIT;
}

void usartDriver_exec(){
	if(modulesState != MODULES_STATE_INIT){
		return;
	}
	uint8_t i;

	if( (usartDriver_transaction_f == false) && (usartDriver_queueCount) ){
		for(i = usartDriver_curentTransactionNum; i < USARTDRIVER_QUEUE_COUNT; i++){
			if(usartDriver_queue[i].state != QUEUE_STATE_NONE){
				usartDriver_curentTransactionNum = i;
				usartDriver_transaction_f = true;
				usartDriver_sendUsartIT();
				return;
			}
		}
		for(i = 0; i < usartDriver_curentTransactionNum; i++){
			if(usartDriver_queue[i].state != QUEUE_STATE_NONE){
				usartDriver_curentTransactionNum = i;
				usartDriver_transaction_f = true;
				usartDriver_sendUsartIT();
				return;
			}
		}
	}
}

bool usartDriver_pushWriteBuff( uint8_t *buff, uint16_t len, void (*fnCallback)(void) ){
	if(modulesState != MODULES_STATE_INIT){
		return false;
	}
	uint8_t i;

	if( (buff == NULL) || (len == 0) || (usartDriver_queueCount >= USARTDRIVER_QUEUE_COUNT)){
		return false;
	}

	for(i = usartDriver_curentTransactionNum; i < USARTDRIVER_QUEUE_COUNT; i++){
		if(usartDriver_queue[i].state == QUEUE_STATE_NONE){
			usartDriver_queue[i].state = QUEUE_STATE_WRITE;
			memcpy(usartDriver_queue[i].buff, buff, len);
			usartDriver_queue[i].len = len;
			usartDriver_queue[i].fnCallback = fnCallback;
			usartDriver_queue[i].count = 0;
			usartDriver_queueCount++;
			return true;
		}
	}
	for(i = 0; i < usartDriver_curentTransactionNum; i++){
		if(usartDriver_queue[i].state == QUEUE_STATE_NONE){
			usartDriver_queue[i].state = QUEUE_STATE_WRITE;
			memcpy(usartDriver_queue[i].buff, buff, len);
			usartDriver_queue[i].len = len;
			usartDriver_queue[i].fnCallback = fnCallback;
			usartDriver_queue[i].count = 0;
			usartDriver_queueCount++;
			return true;
		}
	}
	return false;
}

bool usartDriver_pushReadBuff( void (*fnCallback)(uint8_t) ){
	if(modulesState != MODULES_STATE_INIT){
		return false;
	}
	if(fnCallback == NULL){
		return false;
	}

	if(usartDriver_RX.state != QUEUE_STATE_NONE){
		return false;
	}
	usartDriver_RX.state = QUEUE_STATE_READ;
	usartDriver_RX.fnCallback = fnCallback;
	return false;
}

