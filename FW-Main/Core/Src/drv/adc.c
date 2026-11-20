/*
 * adc.c
 *
 *  Created on: Feb 24, 2024
 *      Author: Sx107
 */

#include <drv/adc.h>
#include <drv/midi.h>

volatile uint16_t adcData1[5*4];
volatile uint16_t adcData2[5*4];
volatile uint16_t adcData[10];

static uint16_t adcv_nocc[10];

#pragma GCC push_options
#pragma GCC optimize ("O0")

void adc_init() {
	// Enable the EOS interrupt in NVIC
	NVIC_SetPriority(ADC1_2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 3));
	NVIC_EnableIRQ(ADC1_2_IRQn);


	// Setup the pins
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOFEN;

	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER0_Msk, (0b11) << GPIO_MODER_MODER0_Pos);
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER1_Msk, (0b11) << GPIO_MODER_MODER1_Pos);
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER2_Msk, (0b11) << GPIO_MODER_MODER2_Pos);
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER3_Msk, (0b11) << GPIO_MODER_MODER3_Pos);

	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER0_Msk, (0b11) << GPIO_MODER_MODER0_Pos);
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER1_Msk, (0b11) << GPIO_MODER_MODER1_Pos);
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER2_Msk, (0b11) << GPIO_MODER_MODER2_Pos);
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER3_Msk, (0b11) << GPIO_MODER_MODER3_Pos);
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER6_Msk, (0b11) << GPIO_MODER_MODER6_Pos);

	MODIFY_REG(GPIOF->MODER, GPIO_MODER_MODER4_Msk, (0b11) << GPIO_MODER_MODER4_Pos);

	// Enable clocks and refgens for adc 1-2
	RCC->CFGR2 |= RCC_CFGR2_ADCPRE12_DIV4;
	RCC->AHBENR |= RCC_AHBENR_ADC12EN;
	ADC12_COMMON->CCR |= 0b00 << ADC12_CCR_CKMODE_Pos;
	//RCC->CFGR2 &= ~RCC_CFGR2_ADCPRE12_Msk;

	// Enable refgen
	ADC1->CR &= ~ADC_CR_ADVREGEN_Msk;
	ADC1->CR |= 0b01 << ADC_CR_ADVREGEN_Pos;
	ADC2->CR &= ~ADC_CR_ADVREGEN_Msk;
	ADC2->CR |= 0b01 << ADC_CR_ADVREGEN_Pos;

	// Wait for 10uS to ensure that all refgens have started
	// At 72MHz 1uS is 72 cycles
	// One cycle takes ~2-3 cycles
	// 10us = 360 cycles
	for(volatile uint16_t i = 0; i < 360; i++) {asm("NOP");}

	// Calibrate ADC1 and ADc2
	ADC1->CR &= ~ADC_CR_ADCALDIF;
	ADC1->CR |= ADC_CR_ADCAL;
	WAITNBIT(ADC1->CR, ADC_CR_ADCAL);

	ADC2->CR &= ~ADC_CR_ADCALDIF;
	ADC2->CR |= ADC_CR_ADCAL;
	WAITNBIT(ADC2->CR, ADC_CR_ADCAL);

	// Configure DMA1 for ADC1
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
	DMA1_Channel1->CMAR = (uint32_t)(adcData1);
	DMA1_Channel1->CNDTR = 5*4;
	DMA1_Channel1->CCR |= DMA_CCR_MINC | (1 << DMA_CCR_MSIZE_Pos) | (1 << DMA_CCR_PSIZE_Pos) | DMA_CCR_CIRC | (3 << DMA_CCR_PL_Pos);
	__DSB();
	DMA1_Channel1->CCR |= DMA_CCR_EN;

	// Configure DMA2 for ADC2
	RCC->AHBENR |= RCC_AHBENR_DMA2EN;
	DMA2_Channel1->CPAR = (uint32_t) (&(ADC2->DR));
	DMA2_Channel1->CMAR = (uint32_t) (adcData2);
	DMA2_Channel1->CNDTR = 5*4;
	DMA2_Channel1->CCR |= DMA_CCR_MINC | (1 << DMA_CCR_MSIZE_Pos) | (1 << DMA_CCR_PSIZE_Pos) | DMA_CCR_CIRC | (3 << DMA_CCR_PL_Pos);
	__DSB();
	DMA2_Channel1->CCR |= DMA_CCR_EN;

	// Setup the dual mode
	ADC12_COMMON->CCR |= (0b0111 << ADC12_CCR_MULTI_Pos) | ADC12_CCR_DMACFG;

	// ADC1: 12 bit, right align, no LP mode,
	ADC1->CFGR |= (0b00 << ADC_CFGR_RES_Pos) | (0b0 << ADC_CFGR_ALIGN_Pos) | (0b0 << ADC_CFGR_AUTDLY_Pos);
	ADC1->CFGR |= (0b1 << ADC_CFGR_CONT_Pos) | (0b0 << ADC_CFGR_OVRMOD_Pos);
	ADC1->CFGR |= ADC_CFGR_DMACFG;
	ADC1->IER |= ADC_IER_OVRIE;

	// ADC2: 12 bit, right align, no LP mode
	ADC2->CFGR |= (0b00 << ADC_CFGR_RES_Pos) | (0b0 << ADC_CFGR_ALIGN_Pos) | (0b0 << ADC_CFGR_AUTDLY_Pos);
	ADC2->CFGR |= (0b1 << ADC_CFGR_CONT_Pos) | (0b0 << ADC_CFGR_OVRMOD_Pos);
	ADC2->CFGR |= ADC_CFGR_DMACFG;
	ADC2->IER |= ADC_IER_OVRIE;

	// Channels SMPR
	ADC1->SMPR1 |= (ADC_SMP_TIME << ADC_SMPR1_SMP1_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP2_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP3_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP4_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP5_Pos);
	ADC2->SMPR1 |= (ADC_SMP_TIME << ADC_SMPR1_SMP3_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP6_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP7_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP8_Pos) | (ADC_SMP_TIME << ADC_SMPR1_SMP9_Pos);

	// Setup channels for ADCs
	ADC1->SQR1 |= (4) << ADC_SQR1_L_Pos;
	ADC1->SQR1 |= (1 << ADC_SQR1_SQ1_Pos) | (2 << ADC_SQR1_SQ2_Pos) | (3 << ADC_SQR1_SQ3_Pos) | (4 << ADC_SQR1_SQ4_Pos);
	ADC1->SQR2 |= (5 << ADC_SQR2_SQ5_Pos);
	ADC2->SQR1 |= (4) << ADC_SQR1_L_Pos;
	ADC2->SQR1 |= (3 << ADC_SQR1_SQ1_Pos) | (6 << ADC_SQR1_SQ2_Pos) | (7 << ADC_SQR1_SQ3_Pos) | (8 << ADC_SQR1_SQ4_Pos);
	ADC2->SQR2 |= (9 << ADC_SQR2_SQ5_Pos);

	// Enable ADC1 EOS Interrupt
	ADC1->IER |= ADC_IER_EOSIE;

	// Enable and start ADC1, ADC2
	ADC1->CFGR |= ADC_CFGR_DMAEN;
	ADC2->CFGR |= ADC_CFGR_DMAEN;
	ADC1->CR |= ADC_CR_ADEN;
	ADC2->CR |= ADC_CR_ADEN;
	WAITBIT(ADC1->ISR, ADC_ISR_ADRDY);
	WAITBIT(ADC2->ISR, ADC_ISR_ADRDY);

	ADC2->CR |= ADC_CR_ADSTART;
	ADC1->CR |= ADC_CR_ADSTART;

}

#pragma GCC pop_options



uint16_t adc_get_value(uint8_t v) {
	//uint8_t adc_ccs[] = {72, 76, 77, 78, 79, 74, 70, 0xFF, 0xFF, 71};
	//uint16_t x = adcData[v] + (midi_get_cc(adc_ccs[v]) << 3);
	//if (x > 1023) {x = 1023;}
	//return x;
	return adcData[v];
}

uint16_t adc_raw_value(uint8_t v) {
	return adcv_nocc[v];
}

void ADC1_2_IRQHandler()  {
	uint8_t adc_ccs[] = {72, 76, 77, 78, 79, 74, 70, 0xFF, 0xFF, 71}; // See main.c for CC assigns

	for(uint8_t i = 0; i < 10; i++) {
		uint16_t v = 0;
		if (i < 5) {v = (adcData1[i] + adcData1[i + 5] + adcData1[i + 10] + adcData1[i + 15]) >> 2;}
		else {v = (adcData2[i-5] + adcData2[i-5 + 5] + adcData2[i-5 + 10] + adcData2[i-5 + 15]) >> 2;}

		uint8_t vh = v & 0b11;
		if(vh >= 1 && vh <= 2) {
			adcv_nocc[i] = v >> 2;
		}

		v = adcv_nocc[i] + (midi_get_cc(adc_ccs[i]) << 3);
		if (v > 1023) {v = 1023;}
		adcData[i] = v;
	}

	ADC1->ISR |= ADC_ISR_EOS;
}
