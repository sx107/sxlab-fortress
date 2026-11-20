/*
 * proc_adc.c
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */
#include <drv/rcc.h>
#include <drv/proc_adc.h>

void proc_adc_init() {
	// OPAMP GPIO
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	GPIOB->MODER |= (3 << GPIO_MODER_MODER11_Pos) | (3 << GPIO_MODER_MODER13_Pos); // OPAMP Input
	GPIOB->MODER |= (3 << GPIO_MODER_MODER1_Pos) | (3 << GPIO_MODER_MODER12_Pos); // OPAMP Output

	// OPAMP
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	// OPAMP3
	OPAMP3->CSR |= OPAMP_CSR_OPAMPxEN | OPAMP_CSR_USERTRIM;
	OPAMP3->CSR |= 0b00 << OPAMP_CSR_VPSEL_Pos;
	OPAMP3->CSR |= 0b11 << OPAMP_CSR_VMSEL_Pos;

	// OPAMP3 Cal
	MODIFY_REG(OPAMP3->CSR, OPAMP_CSR_CALSEL_Msk, (0b11) << OPAMP_CSR_CALSEL_Pos | OPAMP_CSR_CALON);
	for(uint8_t cal = 0; cal < 32; cal++) {
		MODIFY_REG(OPAMP3->CSR, OPAMP_CSR_TRIMOFFSETN_Msk, cal << OPAMP_CSR_TRIMOFFSETN_Pos);
		_delayMs(2);
		if((OPAMP3->CSR & OPAMP_CSR_OUTCAL) == 0) {break;}
	}
	MODIFY_REG(OPAMP3->CSR, OPAMP_CSR_CALSEL_Msk, (0b01) << OPAMP_CSR_CALSEL_Pos | OPAMP_CSR_CALON);
	for(uint8_t cal = 0; cal < 32; cal++) {
		MODIFY_REG(OPAMP3->CSR, OPAMP_CSR_TRIMOFFSETP_Msk, cal << OPAMP_CSR_TRIMOFFSETP_Pos);
		_delayMs(2);
		if((OPAMP3->CSR & OPAMP_CSR_OUTCAL) == 0) {break;}
	}
	OPAMP3->CSR &= ~OPAMP_CSR_CALON;

	// OPAMP4
	OPAMP4->CSR |= OPAMP_CSR_OPAMPxEN | OPAMP_CSR_USERTRIM;
	OPAMP4->CSR |= 0b01 << OPAMP_CSR_VPSEL_Pos;
	OPAMP4->CSR |= 0b11 << OPAMP_CSR_VMSEL_Pos;

	// OPAMP4 Cal
	MODIFY_REG(OPAMP4->CSR, OPAMP_CSR_CALSEL_Msk, (0b11) << OPAMP_CSR_CALSEL_Pos | OPAMP_CSR_CALON);
	for(uint8_t cal = 0; cal < 32; cal++) {
		MODIFY_REG(OPAMP4->CSR, OPAMP_CSR_TRIMOFFSETN_Msk, cal << OPAMP_CSR_TRIMOFFSETN_Pos);
		_delayMs(2);
		if((OPAMP4->CSR & OPAMP_CSR_OUTCAL) == 0) {break;}
	}
	MODIFY_REG(OPAMP4->CSR, OPAMP_CSR_CALSEL_Msk, (0b01) << OPAMP_CSR_CALSEL_Pos | OPAMP_CSR_CALON);
	for(uint8_t cal = 0; cal < 32; cal++) {
		MODIFY_REG(OPAMP4->CSR, OPAMP_CSR_TRIMOFFSETP_Msk, cal << OPAMP_CSR_TRIMOFFSETP_Pos);
		_delayMs(2);
		if((OPAMP4->CSR & OPAMP_CSR_OUTCAL) == 0) {break;}
	}
	OPAMP4->CSR &= ~OPAMP_CSR_CALON;

	// Enable clocks and refgens for adc 1-2
	RCC->CFGR2 |= RCC_CFGR2_ADCPRE34_DIV1;
	RCC->AHBENR |= RCC_AHBENR_ADC34EN;
	ADC34_COMMON->CCR |= 0b00 << ADC34_CCR_CKMODE_Pos;
	//RCC->CFGR2 &= ~RCC_CFGR2_ADCPRE12_Msk;

	// Enable refgen
	ADC3->CR &= ~ADC_CR_ADVREGEN_Msk;
	ADC3->CR |= 0b01 << ADC_CR_ADVREGEN_Pos;
	ADC4->CR &= ~ADC_CR_ADVREGEN_Msk;
	ADC4->CR |= 0b01 << ADC_CR_ADVREGEN_Pos;

	// Wait for 10uS to ensure that all refgens have started
	// At 72MHz 1uS is 72 cycles
	// One cycle takes ~2-3 cycles
	// 10us = 360 cycles
	for(volatile uint16_t i = 0; i < 10000; i++) {asm("NOP");}

	// Calibrate ADC3 and ADC4
	ADC3->CR &= ~ADC_CR_ADCALDIF;
	ADC3->CR |= ADC_CR_ADCAL;
	WAITNBIT(ADC3->CR, ADC_CR_ADCAL);

	ADC4->CR &= ~ADC_CR_ADCALDIF;
	ADC4->CR |= ADC_CR_ADCAL;
	WAITNBIT(ADC4->CR, ADC_CR_ADCAL);

	ADC34_COMMON->CCR |= (0b0111 << ADC34_CCR_MULTI_Pos);
	//OPAMP3->CSR |= (0b00) << OPAMP_CSR_VPSEL_Pos;

	// ADC3: 12 bit, right align, no LP mode,
	ADC3->CFGR |= (0b00 << ADC_CFGR_RES_Pos) | (0b0 << ADC_CFGR_ALIGN_Pos) | (0b0 << ADC_CFGR_AUTDLY_Pos);
	ADC3->CFGR |= (0b1 << ADC_CFGR_CONT_Pos) | (0b0 << ADC_CFGR_OVRMOD_Pos);

	// ADC2: 12 bit, right align, no LP mode
	ADC4->CFGR |= (0b00 << ADC_CFGR_RES_Pos) | (0b0 << ADC_CFGR_ALIGN_Pos) | (0b0 << ADC_CFGR_AUTDLY_Pos);
	ADC4->CFGR |= (0b1 << ADC_CFGR_CONT_Pos) | (0b0 << ADC_CFGR_OVRMOD_Pos);

	// Channels SMPR
	ADC3->SMPR1 |= (0b111 << ADC_SMPR1_SMP1_Pos);
	ADC4->SMPR1 |= (0b111 << ADC_SMPR1_SMP3_Pos);

	// Setup channels for ADCs
	ADC3->SQR1 |= (0) << ADC_SQR1_L_Pos;
	ADC3->SQR1 |= (1 << ADC_SQR1_SQ1_Pos);
	ADC4->SQR1 |= (0) << ADC_SQR1_L_Pos;
	ADC4->SQR1 |= (3 << ADC_SQR1_SQ1_Pos);

	// Enable and start ADC1, ADC2
	ADC3->CR |= ADC_CR_ADEN;
	ADC4->CR |= ADC_CR_ADEN;
	WAITBIT(ADC3->ISR, ADC_ISR_ADRDY);
	WAITBIT(ADC4->ISR, ADC_ISR_ADRDY);

	ADC3->CR |= ADC_CR_ADSTART;
	ADC4->CR |= ADC_CR_ADSTART;
}
