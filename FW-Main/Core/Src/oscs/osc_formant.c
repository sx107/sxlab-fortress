/*
 * osc_formant.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */


#include <oscs/osc_formant.h>
#include <osc_lut.h>

const uint32_t formant_f1[] = {82462,57089,44403,44403,63432};
const uint32_t formant_f2[] = {126865,228357,228357,114178,95149};
const uint32_t formant_f3[] = {282274,285446,294961,260073,279102};

const uint8_t formant_f1_a[] = {15, 13, 11, 15, 15};
const uint8_t formant_f2_a[] = {15, 13, 8, 9, 1};
const uint8_t formant_f3_a[] = {5, 4, 4, 1, 5};


uint16_t osc_formant_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {
	static uint32_t phase_carrier = 0;
	static uint8_t prev_pm = 0;
	static uint32_t phase_f1 = 0;
	static uint32_t phase_f2 = 0;
	static uint32_t phase_f3 = 0;

	uint8_t formant_index = (wf * 5) >> 10;
	if(formant_index > 4) {formant_index = 4;}

	phase_carrier += phase_inc;

	uint32_t f1 = formant_f1[formant_index];
	uint32_t f2 = formant_f2[formant_index];
	uint32_t f3 = formant_f3[formant_index];

	f1 = f1 + (f1 >> 3) - ((f1 * timbre) >> 12);
	f2 = f2 + (f2 >> 3) - ((f2 * timbre) >> 12);

	phase_f1 += f1;
	phase_f2 += f2;
	phase_f3 += f3;

	uint8_t pm = (phase_carrier >> 22) & 0b1;
	if(pm != prev_pm) {
		phase_f1 = 0;
		phase_f2 = 0;
		phase_f3 = 0;
	}
	prev_pm = pm;

	int32_t v = 0;
	v += (sine_12((phase_f1 >> 10) & 4095) - 0x8000) * formant_f1_a[formant_index];
	v += (sine_12((phase_f2 >> 10) & 4095) - 0x8000) * formant_f2_a[formant_index];
	v += (sine_12((phase_f3 >> 10) & 4095) - 0x8000) * formant_f3_a[formant_index];

	v >>= 4; // To 12-bit from 16-bit
	v >>= 4; // Amplitude divide
	v >>= 1; // Divide by 4 because volume?

	v+= 2048;

	if(v < 0) {v = 0;}
	if(v > 4095) {v = 4095;}

	return v;


}
