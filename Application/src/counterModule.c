#include "counterModule.h"


#define counter_period (3276 * 2)

static Counter_Error counter_statusError = COUNTER_OK;
static uint32_t counter_i = 0;
static uint32_t counter_handlerI = 0;

static volatile uint64_t counter_counterCommon = 0;


#define counter_numberHandlerInterrupts 10
static counter_timerInterruptHandler counter_ListHandlersInterrupt[counter_numberHandlerInterrupts];

static volatile bool counter_isFailValueCounter = false;

static bool counter_isFirstInit = true;
//
//void Counter_MspDeInit(){
//    __HAL_RCC_TIM6_CLK_DISABLE();
//    HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn);
//}

void Counter_LPTIMInterrupt(){
	cc1101_sendSync();
	counter_counterCommon += counter_period;
	for(counter_handlerI = 0; counter_handlerI < counter_numberHandlerInterrupts; counter_handlerI++){
	  if(counter_ListHandlersInterrupt[counter_handlerI].isEnable){
		  if(counter_ListHandlersInterrupt[counter_handlerI].timeStartInterrupt <= Counter_GetValueCounter()){
			  (*counter_ListHandlersInterrupt[counter_handlerI].pointerFunction)();
			  counter_ListHandlersInterrupt[counter_handlerI].timeStartInterrupt =
					  counter_ListHandlersInterrupt[counter_handlerI].interval + counter_ListHandlersInterrupt[counter_handlerI].timeStartInterrupt;
		  }
	  }
	}
	counter_isFailValueCounter = true;
}

static void counter_InitLPTIM(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);
    /* Peripheral interrupt init */

    LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
    LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV1);

    LL_LPTIM_Enable(LPTIM1);
    LL_LPTIM_SetAutoReload(LPTIM1, counter_period - 1);
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

    LL_LPTIM_EnableIT_ARRM(LPTIM1);


    NVIC_SetPriority(LPTIM1_IRQn, 1);
    NVIC_EnableIRQ(LPTIM1_IRQn);
}

static void counter_InitInterrupts(void)
{
	for(counter_i = 0; counter_i < counter_numberHandlerInterrupts; counter_i++){
		counter_ListHandlersInterrupt[counter_i].interval = 0;
		counter_ListHandlersInterrupt[counter_i].isEnable = false;
		counter_ListHandlersInterrupt[counter_i].timeStartInterrupt = 0;
	}
}

void Counter_Init (void)
{
	counter_counterCommon = 0;
	if (counter_isFirstInit){
		counter_InitInterrupts();
		counter_isFirstInit = false;
	}
	counter_InitLPTIM();
}

void Counter_RemoveInterruptHandler(void (*function)(void)){
	for(counter_i = 0; counter_i < counter_numberHandlerInterrupts; counter_i++){
		if(counter_ListHandlersInterrupt[counter_i].pointerFunction == function){
			counter_ListHandlersInterrupt[counter_i].timeStartInterrupt = 0;
			counter_ListHandlersInterrupt[counter_i].isEnable = false;
			return;
		}
	}
}

void Counter_AddInterruptHandler(void (*function)(void), uint64_t interval){
	for(counter_i = 0; counter_i < counter_numberHandlerInterrupts; counter_i++){
		if(counter_ListHandlersInterrupt[counter_i].timeStartInterrupt == 0 && !counter_ListHandlersInterrupt[counter_i].isEnable){
			counter_ListHandlersInterrupt[counter_i].interval = Counter_GetSamplesForMilliseconds(interval);
			counter_ListHandlersInterrupt[counter_i].pointerFunction = function;
			counter_ListHandlersInterrupt[counter_i].timeStartInterrupt = Counter_GetValueCounter() + counter_ListHandlersInterrupt[counter_i].interval;
			counter_ListHandlersInterrupt[counter_i].isEnable = true;
			return;
		}
	}
}

uint64_t Counter_GetSamplesForMilliseconds(uint64_t milliseconds){
	return milliseconds * counter_period / 100;
}

uint64_t Counter_GetMillisecondsForSamples(uint64_t samples){
	return samples * 100 / counter_period;
}

void Counter_DelayMSeconds(uint32_t mSec){
	uint64_t endTime = Counter_GetValueCounter() + (mSec * counter_period / 100);
	while(Counter_GetValueCounter() < endTime);
}

void Counter_removeInterruptHandler(void (*function)(void)){
	counter_statusError = COUNTER_OK;
	for(counter_i = 0; counter_i < counter_numberHandlerInterrupts; counter_i++){
		if(counter_ListHandlersInterrupt[counter_i].pointerFunction == function){
			counter_ListHandlersInterrupt[counter_i].pointerFunction = 0;
			counter_ListHandlersInterrupt[counter_i].interval = 0;
			counter_ListHandlersInterrupt[counter_i].isEnable = false;
			counter_ListHandlersInterrupt[counter_i].timeStartInterrupt = 0;
			return;
		}
	}
	counter_statusError = COUNTER_NOT_FOUND_HANDLER;
}

Counter_Error Counter_getStatusError(){
	return counter_statusError;
}

uint64_t Counter_GetValueCounter(){
	counter_isFailValueCounter = false;

	uint64_t value = counter_counterCommon + LL_LPTIM_GetCounter(LPTIM1);

	if(counter_isFailValueCounter){
		return Counter_GetValueCounter();
	}

	counter_isFailValueCounter = true;
	return value;
}
