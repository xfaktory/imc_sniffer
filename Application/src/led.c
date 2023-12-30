/*
 * led.c
 *
 *  Created on: 25 июл. 2023 г.
 *      Author: xar
 */
#include "led.h"

void led_init(){
	  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	  GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
	  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
