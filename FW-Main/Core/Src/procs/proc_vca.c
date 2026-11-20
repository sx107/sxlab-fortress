/*
 * proc_vca.c
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */


#include <procs/proc_vca.h>
#include <dsp/dsp.h>
#include <osc_lut.h>


uint16_t proc_vca_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {
	if(in2 > 100) {in2 -= 100;}
	else {in2 = 0;}

	int32_t v1 = in1 - 2048;
	v1  = (v1 * vca_gain_lut_12(in2)) >> 16;

	if(knob > 30) {knob -= 30;}
	else {knob = 0;}

	v1 = (v1 * knob) >> 3;
	v1 += 32768;

	if(v1 < 0) {v1 = 0;}
	if(v1 > 65535) {v1 = 65535;}

	v1 = hardclip_lut_16(v1);

	v1 >>= 4;
	return v1;
}

