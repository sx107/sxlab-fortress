/*
 * lfo_timer.h
 *
 *  Created on: Feb 24, 2024
 *      Author: Sx107
 */

#ifndef LFO_TIMER_H_
#define LFO_TIMER_H_

#include <main.h>

#define LFO1OUT (TIM8->CCR1)
#define LFO1RND (TIM8->CCR2)
#define LFO2OUT (TIM8->CCR3)
#define LFO2RND (TIM8->CCR4)

void lfo_timer_init();

#endif /* LFO_TIMER_H_ */
