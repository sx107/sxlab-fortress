/*
 * osc_add1.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */

#include <oscs/osc_add1.h>
#include <osc_lut.h>

#define N_ADD1_SINES 4

uint16_t osc_add1_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {
	static uint32_t phase[5] = {0, 0xFACEFACE, 0xECAFECAF, 0xCAFECAFE, 0xEFACEFAC};

	if(wf > 25) {wf -= 25;}
	else {wf = 0;}




	phase[0] += phase_inc;
	for(uint8_t i = 1; i < N_ADD1_SINES; i++) {
		phase[i] += phase_inc*(1+i) + (phase_inc * i * wf >> 10);
	}

	int32_t v = sine_12((phase[0] >> 10) & 4095) - 0x8000;
	for(uint8_t i = 1; i < N_ADD1_SINES; i++) {
		uint16_t vol = timbre & 0xFF;
		if (timbre < 256 * (i-1)) {vol = 0;}
		if (timbre >= 256 * (i)) {vol = 255;}


		v += ((sine_12((phase[i] >> 10) & 4095) - 0x8000) * vol) >> 8;
	}

	if (timbre < 256) {v += v * (256 - timbre) >> 7;}
	v = v >> 2;
	v = (v >> 4) + 2048;

	if(v < 0) {v = 0;}
	if (v > 4095) {v = 4095;}





	return v;

}
