/*
 * rcc.c
 *
 *  Created on: Aug 6, 2022
 *      Author: Sx107
 */

#include <main.h>


void rcc_init() {
	FLASH->ACR |= FLASH_ACR_LATENCY_2 | FLASH_ACR_PRFTBE;
	WAITBIT(FLASH->ACR, FLASH_ACR_LATENCY_2);

	// Enable HSI
	RCC->CR |= RCC_CR_HSEON;
	WAITBIT(RCC->CR, RCC_CR_HSERDY);

	//Initialize RCC & LSI Here if required

	//Setup the PLL Mul
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL, RCC_CFGR_PLLSRC_HSE_PREDIV | RCC_CFGR_PLLMUL9);
	MODIFY_REG(RCC->CFGR2, RCC_CFGR2_PREDIV, RCC_CFGR2_PREDIV_DIV1);

	// Turn on the PLL
	RCC->CR |= RCC_CR_PLLON;
	WAITBIT(RCC->CR, RCC_CR_PLLRDY);

	// Setup AHB/APB1/APB2 clocks
	MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1);		//AHB
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV2);		//APB1
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV1);		//APB2

	// Select PLL
	MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL);
	WAITVAL(RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL);

	// Set up the systick
	__disable_irq();
	SysTick->LOAD = (uint32_t)(72000 - 1);
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL = 0UL;
	__enable_irq();

	// USB Clock
	MODIFY_REG(RCC->CFGR, RCC_CFGR_USBPRE, RCC_CFGR_USBPRE_DIV1_5);
}

void _delayMs(uint32_t del) {
	SysTick->VAL = 0;
	while (del) {
		if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U)
		{
			del--;
	    }
	}
}

