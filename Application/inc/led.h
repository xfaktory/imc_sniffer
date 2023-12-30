/*
 * led.h
 *
 *  Created on: 25 июл. 2023 г.
 *      Author: xar
 */

#ifndef INC_LED_H_
#define INC_LED_H_

#include "main.h"

#define led_led1On() LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_3)
#define led_led2On() LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7);

#define led_led1Off() LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_3)
#define led_led2Off() LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_7);

#define led_led1Toggle() LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_3)
#define led_led2Toggle() LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_7);

void led_init();

#endif /* INC_LED_H_ */
