#ifndef counterModule_H
#define counterModule_H

#include "main.h"

typedef enum {
	COUNTER_OK,
	COUNTER_NOT_FOUND_HANDLER
}Counter_Error;

typedef struct
{
	void (*pointerFunction)(void);
	uint64_t interval;
	uint64_t timeStartInterrupt;
	bool isEnable;
}counter_timerInterruptHandler;

void Counter_AddInterruptHandler(void (*function)(void), uint64_t interval);

void Counter_RemoveInterruptHandler(void (*function)(void));
void Counter_removeInterruptHandler(void (*function)(void));

void Counter_Init(void);
uint64_t Counter_GetValueCounter();

uint64_t Counter_GetSamplesForMilliseconds(uint64_t milliseconds);

//uint64_t Counter_GetSamplesForMicroseconds(uint64_t microseconds);

uint64_t Counter_GetMillisecondsForSamples(uint64_t samples);

//double Counter_GetMicrosecondsForSamples(uint64_t samples);

void Counter_DelayMSeconds(uint32_t mSec);

//void Counter_DelayMCSeconds(uint32_t mcSec);

Counter_Error Counter_getStatusError();
void Counter_LPTIMInterrupt();

#endif
