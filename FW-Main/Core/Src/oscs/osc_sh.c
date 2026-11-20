/*
 * osc_am.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */

#include <osc_lut.h>
#include <oscs/osc_sh.h>
#include <dsp/dsp.h>

uint16_t osc_sh_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {
	static uint32_t phase_main = 0;
	static uint32_t phase_sh = 0;
	static uint8_t prev_pm;
	static uint16_t output = 0;

	phase_main += phase_inc;
	phase_sh += pitchKnobInc_10(timbre);

	uint8_t pm = (phase_sh >> 19) & 0b1;
	uint16_t v_sine = sine_12((phase_main >> 10) & 4095) >> 4;
	uint16_t v_saw = (phase_main >> 10) & 4095;

	if(pm != prev_pm) {
		output = v_sine + (((v_saw - v_sine) * wf) >> 10);
	}
	prev_pm = pm;

	return output;
}
