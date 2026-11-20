/*
 * led.h
 *
 * Library for APA102/Sk9822 LED
 *
 *  Created on: 16 мая 2021 г.
 *      Author: Sx107
 */

#ifndef LED_H_
#define LED_H_

#include "main.h"
#define LED_NUMBER 23

#define SPI1_DMA DMA1
#define SPI1_DMARX DMA1_Channel2
#define SPI1_DMATX DMA1_Channel3

void led_init();
void led_clear(uint8_t led_id);
void led_set(uint8_t led_id, uint32_t color, uint8_t brightness);
void led_update();

#define LED_BRIGHTNESS 10
#define LED_SEL_BRIGHTNESS 1

#define LED_LFO2_2 0
#define LED_LFO2_1 1
#define LED_LFO2_0 2

#define LED_LFO1_2 3
#define LED_LFO1_1 4
#define LED_LFO1_0 5

#define LED_VCO_PM_0	6
#define LED_VCO_PM_1	7

#define LED_VCO_M_0		8
#define LED_VCO_M_1		9
#define LED_VCO_M_2		10
#define LED_VCO_M_3		11

#define LED_PROC_KNOB 12

#define LED_PROC_0 13
#define LED_PROC_1 14
#define LED_PROC_2 15
#define LED_PROC_3 16
#define LED_PROC_4 17

#define LED_PROC_5 22
#define LED_PROC_6 22
#define LED_PROC_7 20
#define LED_PROC_8 19
#define LED_PROC_9 18


#endif /* LED_H_ */
