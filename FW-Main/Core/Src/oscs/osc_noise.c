/*
 * osc_noise.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */

#include <oscs/osc_noise.h>
#include <rnd.h>


uint16_t osc_noise_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {

	static uint32_t phase = 0;

	static uint32_t noise = 0xFACEECAF;
	static uint32_t phase_sh = 0;
	static uint8_t prev_pm = 0;
	static uint16_t n_sh = 0;

	noise = xorshift32(noise);

	phase += phase_inc  << (timbre >> 8);

	int32_t n = ((phase >> (wf >> 8)) & 4095) - 2048;
	n = (n << 4) + (10 * n * timbre >> 4);
	if(n > 4095) {n = 4095;}
	if(n < 0) {n = 0;}

	uint16_t n2 = noise & 4095;
	if(phase_inc > 50000) {phase_inc = 50000;}
	phase_sh += (phase_inc * phase_inc) >> 16;
	uint8_t pm = (phase_sh >> 16) & 0b1;
	if(pm != prev_pm) {
		n_sh = n2;
	}
	prev_pm = pm;

	uint32_t output = n_sh + (((n - n_sh) * wf) >> 10);

	return  output;
}
