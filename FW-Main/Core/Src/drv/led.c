/*
 * led.c
 *
 *  Created on: 16 мая 2021 г.
 *      Author: Sx107
 */


#include "drv/led.h"

uint32_t led_buffer[LED_NUMBER];

__attribute__ ((aligned (4))) uint32_t led_buffer_dma[LED_NUMBER+2];


void led_clear(uint8_t led_id) {
	led_set(led_id, 0, 0);
}
void led_set(uint8_t led_id, uint32_t color, uint8_t brightness) {
	uint32_t q = (0b11100000 << 24) | ((brightness & 31) << 24) | (color & 0x00FFFFFF);

	led_buffer_dma[led_id+1] = ((q << 16) | (q >> 16));


	//led_buffer_dma[led_id+1] = 0x0000FF00;

}

void led_init() {
	led_buffer_dma[0] = 0x00000000;
	led_buffer_dma[LED_NUMBER+2 - 1] = 0xFFFFFFFF;
	for(uint8_t i = 0; i < LED_NUMBER; i++) {
		led_clear(i);
	}

	// PB5 Data, PB3 Clock
	// Use SPI1
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER3_Msk, (0b10) << GPIO_MODER_MODER3_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER5_Msk, (0b10) << GPIO_MODER_MODER5_Pos);
	GPIOB->OTYPER &= ~(GPIO_OTYPER_OT_3 | GPIO_OTYPER_OT_5); // Open-drain
	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR3_Msk | GPIO_OSPEEDER_OSPEEDR5_Msk; // Max Speed
	GPIOB->AFR[0] |= (5 << GPIO_AFRL_AFRL3_Pos) | (5 << GPIO_AFRL_AFRL5_Pos); // AF5 SPI1

	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	MODIFY_REG(SPI1->CR1, 0xFFFF, ((0b010) << SPI_CR1_BR_Pos) | SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI);
	MODIFY_REG(SPI1->CR2, 0xFFFF, (0b1111) << SPI_CR2_DS_Pos);
	SPI1->CR1 |= SPI_CR1_SPE;

	WAITNBIT(SPI1->SR, SPI_SR_BSY);
	SPI1->CR2 &= ~(SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN); __DSB();

	SPI1_DMATX->CCR &= ~DMA_CCR_EN;
	SPI1_DMATX->CCR &= ~(DMA_CCR_TCIE | DMA_CCR_HTIE | DMA_CCR_TEIE);
	__DSB();

	SPI1_DMATX->CCR = 0;
	SPI1_DMATX->CNDTR = (LED_NUMBER+2) * 2;
	SPI1_DMATX->CMAR = (uint32_t) (led_buffer_dma);
	SPI1_DMATX->CPAR = (uint32_t) (&(SPI1->DR));
	SPI1_DMATX->CCR |= 0b01 << DMA_CCR_MSIZE_Pos;
	SPI1_DMATX->CCR |= 0b01 << DMA_CCR_PSIZE_Pos;
	SPI1_DMATX->CCR |= 0b1 << DMA_CCR_CIRC_Pos;
	__DSB();
	SPI1_DMATX->CCR |= (0b01 << DMA_CCR_DIR_Pos) | DMA_CCR_MINC | DMA_CCR_EN | (0b00 << DMA_CCR_PL_Pos);
	__DSB();

	SPI1->CR2 |= SPI_CR2_TXDMAEN;
}

void led_update() {
	/*
	for(uint16_t i = 0; i < LED_NUMBER+2; i++) {
		uint32_t q = (led_buffer_dma[i] >> 16) & 0x0000FFFF;
		SPI1->DR = q;
		WAITBIT(SPI1->SR, SPI_SR_TXE);
		(void) SPI1->DR;

		q = (led_buffer_dma[i] >> 0) & 0x0000FFFF;
		SPI1->DR = q;
		WAITBIT(SPI1->SR, SPI_SR_TXE);
		(void) SPI1->DR;
	}*/


}

/*

void led_init() {
	// PB5 Data, PB3 Clock
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER3_Msk, (0b01) << GPIO_MODER_MODER3_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER5_Msk, (0b01) << GPIO_MODER_MODER5_Pos);

	GPIOB->OTYPER |= GPIO_OTYPER_OT_3 | GPIO_OTYPER_OT_5; // Open-drain

	GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR3_Msk | GPIO_OSPEEDER_OSPEEDR5_Msk; // Max Speed

	for(uint8_t i = 0; i < LED_NUMBER; i++) {
		led_clear(i);
	}
}

void led_update() {
	TIM15->SR &= ~TIM_SR_UIF;
	GPIOB->BSRR |= GPIO_BSRR_BR_3;

	// 1. Empty (0x00) send, start condition
	GPIOB->BSRR |= GPIO_BSRR_BR_5;
	for(uint8_t i = 0; i < 32; i++) {
		GPIOB->BSRR |= GPIO_BSRR_BS_3;
		GPIOB->BSRR |= GPIO_BSRR_BR_3;
	}

	// 2. Send the LEDs
	for(uint16_t l = 0; l < LED_NUMBER; l++) {
		for(uint8_t i = 0; i < 32; i++) {

			GPIOB->BSRR |= ((led_buffer[l] >> (31 - i)) & 0b1) == 1 ? GPIO_BSRR_BS_5 : GPIO_BSRR_BR_5;

			GPIOB->BSRR |= GPIO_BSRR_BS_3;
			GPIOB->BSRR |= GPIO_BSRR_BR_3;
		}
	}

	// 3. End condition
	GPIOB->BSRR |= GPIO_BSRR_BS_5;
	for(uint8_t i = 0; i < 32; i++) {
		GPIOB->BSRR |= GPIO_BSRR_BS_3;
		GPIOB->BSRR |= GPIO_BSRR_BR_3;
	}
	GPIOB->BSRR |= GPIO_BSRR_BR_5;
}

*/
