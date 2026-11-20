/*
 * lfo2.h
 *
 *  Created on: Feb 24, 2024
 *      Author: Sx107
 */

#ifndef LFO2_H_
#define LFO2_H_

#include <main.h>

void lfo2_init();
void lfo2_process() __attribute__((section (".ccmram")));
void lfo2_update();

void lfo2_reset();
void lfo2_full_reset();
void lfo2_midi_reset();
#endif /* LFO2_H_ */
