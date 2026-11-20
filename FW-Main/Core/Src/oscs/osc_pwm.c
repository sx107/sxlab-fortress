/*
 * osc_pwm.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */

#include <oscs/osc_pwm.h>

uint16_t osc_pwm_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {
	static uint32_t phase_sync = 0, phase_main = 0;
	static uint8_t prev_pm_22 = 0;

	phase_main += phase_inc;
	phase_sync += phase_inc  + ((8 * phase_inc * timbre) >> 10);

	uint8_t pm_22 = (phase_main >> 22) & 0b1;

	if(pm_22 != prev_pm_22) {
		//phase_main = 0;
		phase_sync = 0;
	}

	prev_pm_22 = pm_22;

	int16_t pw = 2048 - (wf << 1);
	if(pw < 250) {pw = 250;}

	return ((phase_sync >> 10) & 4095) > pw ? 0 : 4095;

}
