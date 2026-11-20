/*
 * osc_fm.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */

#include <oscs/osc_fm.h>
#include <osc_lut.h>

uint16_t osc_fm_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {
	static uint32_t p_car = 0;
	static uint32_t p_mod1 = 0;
	static uint32_t p_mod2 = 0;

	uint16_t mod_amount_1 = timbre > 512 ? 512 : timbre;
	uint16_t mod_amount_2 = timbre >= 512 ? timbre - 512 : 0;

	int32_t mod1 = sine_12((p_mod1 >> 10) & 4095) - 0x8000;
	int32_t mod2 = sine_12((p_mod2 >> 10) & 4095) - 0x8000;

	p_car += phase_inc + ((mod1*mod_amount_1) >> 6)  + ((mod2*mod_amount_2) >> 7);
	p_mod1 += phase_inc + phase_inc * (wf >> 7);
	p_mod2 += phase_inc + phase_inc * (1 + (wf >> 8));


	return sine_12((p_car >> 10) & 4095) >> 4;
}
