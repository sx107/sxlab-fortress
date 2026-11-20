/*
 * osc_morph.c
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */


#include <oscs/osc_morph.h>
#include <osc_lut.h>

inline static int32_t wavefold(int32_t v, uint16_t timbre) {
	int32_t vp = v - 2048;

	vp = vp + ((10 * vp * (timbre)) >> 10);

	if (vp > 0) {
		v = vp & 2047;
		if(((vp >> 11) & 0b1) == 0) {vp = v;}
		else {vp = 2047 - v;}
	} else {
		vp = -vp;
		v = vp & 2047;
		if(((vp >> 11) & 0b1) == 0) {vp = v;}
		else {vp = 2047 - v;}
		vp = -vp;
	}

	v = vp + 2048;

	return v;
}

uint16_t osc_morph_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) {
	static uint32_t phase;

	int32_t v = 2048;

	phase += phase_inc;

	uint16_t ph12 = (phase >> 10) & 4095;

	if (wf < 128) {
		// Sine -> Tri
		// The triangle
		if(((ph12 >> 11) & 0b1) == 0) {v = (phase >> 9) & 4095;}
		else {v = 4095 - ((phase >> 9) & 4095);}

		uint16_t vs = sine_clip_12(v) >> 4;
		v = vs + (((v - vs) * wf) >> 7);
	} else if (wf < 256) {
		// Tri -> Saw
		uint16_t t_p = 2048 + ((wf - 128) << 4);
		if(ph12 < t_p) {
			v = (ph12) * 4095 / (t_p);
		} else {
			v = 4095 - (ph12-(t_p)) * 4095 / (4095-t_p);
		}
	} else if (wf < 512) {
		// Saw -> Trumpet
		uint16_t t_p = (wf - 256) << 3;
		if(ph12 > t_p) {
			v = (ph12-t_p) * 4095 / (4095-t_p);
		} else {
			v = 2048;
		}
	} else if (wf < 768) {
		/*
		// Trumpet -> Square
		uint16_t v_trumpet;
		uint16_t v_square;
		if(((ph12 >> 11) & 0b1) == 1) {
			v_trumpet = (phase >> 9) & 4095;
			v_square = 4095;
		} else {
			v_trumpet = 2048;
			v_square = 0;
		}

		v = v_trumpet + (((v_square - v_trumpet) * (wf - 512)) >> 8);
		*/

		uint16_t v_trumpet;
		uint16_t v_tri;

		if(((ph12 >> 11) & 0b1) == 0) {v_tri = (phase >> 9) & 4095;}
		else {v_tri = 4095 - ((phase >> 9) & 4095);}

		if(((ph12 >> 11) & 0b1) == 1) {
			v_trumpet = (phase >> 9) & 4095;
		} else {
			v_trumpet = 2048;
		}

		v_tri = wavefold(v_tri, timbre >> 1);
		v_tri = v_tri > 2048 ? 0 : 4095;

		v_trumpet = wavefold(v_trumpet, timbre);

		v = v_trumpet + (((v_tri - v_trumpet) * (wf - 512)) >> 8);

	} else {
		// PWM
		/*
		uint16_t t_p = 2048 + ((wf - 768) << 3);
		if(t_p > 4095 - 250) {t_p = 4095 - 250;}
		if(ph12 < t_p) {v = 0;}
		else {v = 4095;}
		*/

		if(((ph12 >> 11) & 0b1) == 0) {v = (phase >> 9) & 4095;}
		else {v = 4095 - ((phase >> 9) & 4095);}

		v = wavefold(v, timbre >> 1);

		uint16_t pwm = (wf - 768) << 3;
		if (pwm > 2048 - 250) {pwm = 2048 - 250;}

		v = v > 2048 + pwm ? 0 : 4095;
	}

	if (wf < 512) {
		v = wavefold(v, timbre);
	}

	return v;
}
