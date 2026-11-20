/*
 * led.c
 *
 *  Created on: 16 мая 2021 г.
 *      Author: Sx107
 */


#include "drv/led.h"

uint32_t led_buffer[LED_NUMBER];

void led_clear(uint8_t led_id) {
	led_set(led_id, 0, 0);
}
void led_set(uint8_t led_id, uint32_t color, uint8_t brightness) {
	led_buffer[led_id] = (0b11100000 << 24) | (color & 0x00FFFFFF) | ((brightness & 31) << 24);
}

void led_init() {
	// PB5 Data, PB3 Clock
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER3_Msk, (0b01) << GPIO_MODER_MODER3_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER5_Msk, (0b01) << GPIO_MODER_MODER5_Pos);

	//GPIOB->OTYPER |= GPIO_OTYPER_OT_3 | GPIO_OTYPER_OT_5; // Open-drain

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
