/*
 * osc_add2.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */

#include <oscs/osc_add2.h>
#include <osc_lut.h>

#define N_ADD2_SINES 4

const uint8_t add2_freq_mult[24]=
			{	2,	2,	2,	3,	1,	2,	4,	4,
				3,	4,	5,	2,	1,	5,	6,	8,
				3,	8,	7,	8,	7,	6,	8,	16};

uint16_t osc_add2_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {
	static uint32_t phase[5] = {0xFACEFACE, 0xECAFECAF, 0xCAFEECAF, 0xECAFCAFE, 0xFACE8D8D};

	phase[0] += phase_inc;
	for(uint8_t i = 1; i < N_ADD2_SINES; i++) {
		int32_t detune = (-1440 + i * 960) * wf >> 10;
		phase[i] += (phase_inc + detune) * add2_freq_mult[(i-1)*8 + (timbre >> 7)];
	}

	uint32_t v = 0;
	for(uint8_t i = 0; i < N_ADD2_SINES; i++) {
		v += (sine_12(4095 & (phase[i] >> 10))) >> 4;
	}
	v >>= 2;

	return v;
}
