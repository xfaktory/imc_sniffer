/*
 * pwr.c
 *
 *  Created on: 11 июл. 2023 г.
 *      Author: xar
 */

#include "pwr.h"

#define PWR_SLEEP_TIMEOUT 10000

static uint64_t pwr_sleepTimer = 0;

static void pwr_sleepWFI(){
	uint32_t ulpbit;
	uint32_t vrefinbit;
	uint32_t tmpreg;

	ulpbit = READ_BIT(PWR->CR, PWR_CR_ULP);
	vrefinbit = READ_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_EN_VREFINT);
	if((ulpbit != 0) && (vrefinbit != 0)){
		CLEAR_BIT(PWR->CR, PWR_CR_ULP);
	}

	tmpreg = PWR->CR;
	CLEAR_BIT(tmpreg, (PWR_CR_PDDS));
	SET_BIT(tmpreg, PWR_CR_LPSDSR);

	PWR->CR = tmpreg;

	CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);

	if((ulpbit != 0) && (vrefinbit != 0))
	{
		SET_BIT(PWR->CR, PWR_CR_ULP);
	}

	__WFI();
	__NOP();
}

void pwr_turnOn(){
	cc1101_turnOn();
}

void pwr_turnOff(){
	cc1101_turnOff();
	pwr_sleepWFI();
}

void pwr_sleep(){
	pwr_sleepWFI();
}

void pwr_exec(){

}

void pwr_init(){
	pwr_sleepTimer = 0;
}
