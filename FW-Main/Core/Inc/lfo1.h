/*
 * lfo1.h
 *
 *  Created on: Feb 25, 2024
 *      Author: Sx107
 */

#ifndef LFO1_H_
#define LFO1_H_

#include <main.h>

void lfo1_init();
void lfo1_process() __attribute__((section (".ccmram")));
void lfo1_update();

void lfo1_reset();
void lfo1_full_reset();

#endif /* LFO1_H_ */
