/*
 * proc_lfo.c
 *
 *  Created on: 5 мар. 2024 г.
 *      Author: Sx107
 */


#include <procs/proc_lfo.h>

const uint16_t proc_lfo_freq_lut[] = {191, 209, 229, 250, 274, 300, 329, 360, 394, 431, 472, 517, 566, 619, 678, 742, 813, 890, 974, 1066, 1167, 1278, 1399, 1532, 1677, 1836, 2009, 2200, 2408, 2637, 2886, 3160, 3459, 3787, 4146, 4539, 4969, 5440, 5955, 6520, 7138, 7814, 8554, 9365, 10253, 11224, 12288, 13452, 14727, 16122, 17650, 19323, 21154, 23158, 25353, 27755, 30385, 33265, 36417, 39868, 43646, 47782, 52309, 57266, 62693};

uint16_t proc_lfo_inc(const uint16_t v) {
	uint32_t lut_low = proc_lfo_freq_lut[v >> 4];
	uint32_t lut_high = proc_lfo_freq_lut[(v >> 4) + 1];
	return lut_low + (((lut_high - lut_low) * (v & 0b1111)) >> 4);
}

uint16_t proc_lfo_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {
	static uint32_t phase = 0;
	static uint8_t reset_t = 0;
	static uint8_t prev_reset = 0;

	if(in1 > 3500) {reset_t = 1;}
	else if (in1 < 1000) {reset_t = 0;}

	if(reset_t && !prev_reset) {
		phase = 0;
	}
	prev_reset = reset_t;


	phase += proc_lfo_inc(knob);
	uint16_t lfoPhaseQ = 8191 & (phase >> (24-13));
	uint16_t v = 0;


	if(lfoPhaseQ >> 12) {
		v = 4095 - (lfoPhaseQ - 4096);
	} else {
		v = lfoPhaseQ;
	}

	return v;
}
