/*
 * update.c
 *
 *  Created on: Feb 26, 2024
 *      Author: Sx107
 */


#include <drv/update.h>
#include <drv/led.h>
#include <lfo1.h>
#include <lfo2.h>
#include <osc_proc_core.h>

void update_init() {
	NVIC_SetPriority(TIM1_BRK_TIM15_IRQn,  NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 3, 3));
	NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);

	RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
	TIM15->PSC = 1;
	TIM15->ARR = 31999;
	TIM15->CR1 |= TIM_CR1_ARPE;
	TIM15->DIER |= TIM_DIER_UIE;
	__DSB();
	TIM15->CR1 |= TIM_CR1_CEN;
}

void TIM1_BRK_TIM15_IRQHandler() {
	TIM15->SR &= ~TIM_SR_UIF;
	lfo1_update();
	lfo2_update();
	osc_proc_update();
}
