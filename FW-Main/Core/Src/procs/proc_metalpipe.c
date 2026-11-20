/*
 * proc_metalpipe.c
 *
 *  Created on: 7 апр. 2025 г.
 *      Author: Sx107
 */


#include <procs/proc_metalpipe.h>
#include <dsp/dsp.h>
#include <osc_lut.h>



uint16_t proc_metalpipe_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {
	static float sndpos = 0;
	static uint8_t sndplay = 0;
	static float sound_reverse = 1;
	static uint8_t* snd_addr = sound_sample;

	static uint8_t prev_trigger = 0;
	static uint8_t dt = 0;

	if(in1 > 3500) {dt = 1;}
	if(in1 < 1000) {dt = 0;}
	if(dt && !prev_trigger) {
		if(in2 > 2500) {
			snd_addr = sound_sample;
			sndpos = 0;
			//sndpos = MP_SMPSIZE - 20;
			//sound_reverse = -1;
		} else {
			snd_addr = (uint8_t*)0x08000000;
			sndpos = 0;
			//sndpos = 0;
			//sound_reverse = 1;
		}
		sndplay = 1;
	}
	prev_trigger = dt;

	if (sndpos < 0 || (uint16_t)sndpos >= MP_SMPSIZE || !sndplay) {
		sndplay = 0;
		sndpos = 0;
		return 2048;
	}
	uint16_t ret =  2048;
	if(snd_addr == sound_sample) {
		ret = snd_addr[(uint16_t)sndpos] << 4;
	} else {
		ret =  ((uint16_t*)snd_addr)[(uint16_t)sndpos] >> 4;
	}
	sndpos += sound_reverse * ((float)knob / 1024);
	return ret;
}

void proc_metalpipe_update(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {

}
