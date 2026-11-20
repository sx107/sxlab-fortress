/*
 * buttons.h
 *
 *  Created on: 14 июл. 2025 г.
 *      Author: Sx107
 */

#ifndef INC_DRV_BUTTONS_H_
#define INC_DRV_BUTTONS_H_

#include <main.h>

void buttons_init();

#define LFO1BUT (!(GPIOC->IDR & GPIO_IDR_10))
#define LFO2BUT (!(GPIOC->IDR & GPIO_IDR_11))
#define BUT_VCO (!(GPIOC->IDR & GPIO_IDR_4))
#define BUT_PROC (!(GPIOC->IDR & GPIO_IDR_5))

#endif /* INC_DRV_BUTTONS_H_ */
