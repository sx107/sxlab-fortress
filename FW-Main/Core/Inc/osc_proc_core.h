/*
 * osc_proc_core.h
 *
 *  Created on: 28 февр. 2024 г.
 *      Author: Sx107
 */

#ifndef INC_OSC_PROC_CORE_H_
#define INC_OSC_PROC_CORE_H_

#include <main.h>

typedef uint16_t (*osc_engine_proc)(uint32_t, uint16_t, uint16_t, uint16_t);
typedef uint16_t (*proc_engine_proc)(uint8_t, uint16_t, uint16_t, uint16_t);
typedef void (*proc_engine_update)(uint8_t, uint16_t, uint16_t, uint16_t);

float osc_inc(float freq);

void osc_proc_init();
void osc_proc_update();

void TIM7_IRQHandler() __attribute__((section (".ccmram")));

#endif /* INC_OSC_PROC_CORE_H_ */
