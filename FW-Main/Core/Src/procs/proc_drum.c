/*
 * proc_drum.c
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */

/*
 * Drum ID:
 * 0 - Silence
 * 1 - Bass drum
 * 2 - Snare
 * 3 - Closed hihat
 * 4 - Noise
 * 5 - Open hihat
 * 6 - Closed hihat
 * 7 - Silence
 */

#include <procs/proc_drum.h>
#include <rnd.h>
#include <osc_lut.h>

volatile uint32_t _p_drum_decay = 0;
volatile uint32_t _p_drum_snare_inc = 0;


uint16_t proc_drum_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {
	static uint8_t prev_trigger = 0;
	static uint8_t drum_id = 0;
	static int32_t drum_decay = 0;
	static uint8_t dt = 0;

	static uint32_t noise = 0xFACEECAF;

	static uint32_t snare_phase = 0;
	static uint32_t kick_phase = 0;
	static uint32_t hh_phase[4] = {0, 0, 0, 0};
	const uint32_t hh_incs[4] = {548527,391163,151498,221962};

	if(in1 > 3500) {dt = 1;}
	if(in1 < 1000) {dt = 0;}
	if(dt && !prev_trigger) {
		drum_decay = 0xFFFFFF;
		kick_phase = 0;
		drum_id = (knob) >> 7;

		//drum_id = 1;
	}
	prev_trigger = dt;

	if (drum_decay <= 0) {return 2048;}

	int32_t drum_sample = 0;

	if(drum_id == 1) {
		// Bass drum
		kick_phase += pitchKnobInc_10(50+(drum_decay >> 15));
		drum_sample = sine_12((kick_phase >> 10) & 4095) - 0x8000;
		drum_sample = (drum_sample * (drum_decay >> 14)) >> 10;
		drum_sample >>= 4;

		//if(drum_sample > 2047) {drum_sample = 2047;}
		//else if(drum_sample < -2048) {drum_sample = -2048;}

		drum_sample += 2048;
	}
	else if(drum_id == 2) {
		// Snare
		noise = xorshift32(noise);
		drum_sample = ((noise >> 16) - 0x8000);
		if(drum_sample > 1023) {drum_sample = 1023;}
		else if(drum_sample < -1024) {drum_sample = -1024;}

		drum_sample += (sine_12((snare_phase >> 10) & 4095) - 0x8000) >> 4;
		snare_phase += _p_drum_snare_inc;

		drum_sample = (drum_sample * (drum_decay >> 14)) >> 10;

		if(drum_sample > 2047) {drum_sample = 2047;}
		else if(drum_sample < -2048) {drum_sample = -2048;}

		drum_sample += 2048;
	}
	else if (drum_id == 3 || drum_id == 5 || drum_id == 6) {
		// Hihat - open or closed
		// Closed hihat deacy /= 2
		if(drum_id == 3 || drum_id == 6) {
			drum_decay -= _p_drum_decay;
			if(drum_decay < 0) {drum_decay = 0;}
		}

		drum_sample = 0;
		for(uint8_t i = 0; i < 4; i++) {
			hh_phase[i] += hh_incs[i];
			drum_sample += ((hh_phase[i] >> 11) & 1023) > 900 ? 1023 : -1024;
		}

		noise = xorshift32(noise);
		drum_sample += (noise >> 24) - 127;

		drum_sample = (drum_sample * (drum_decay >> 14)) >> 10;

		if(drum_sample > 2047) {drum_sample = 2047;}
		else if(drum_sample < -2048) {drum_sample = -2048;}

		drum_sample += 2048;

	}
	else if(drum_id == 4) {
		// Noise
		drum_decay -= _p_drum_decay;
		if(drum_decay < 0) {drum_decay = 0;}

		noise = xorshift32(noise);
		drum_sample = (noise >> 16) - 0x8000;

		drum_sample = (drum_sample * (drum_decay >> 14)) >> 10;
		drum_sample = (drum_sample >> 4) + 2048;
	}
	else {
		// Silence
		drum_sample = 2048;
	}






	drum_decay -= _p_drum_decay;
	if(drum_decay < 0) {drum_decay = 0;}

	return drum_sample;
}



void proc_drum_update(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {
	_p_drum_decay = pitchKnobInc_10(512 - (in2 >> 3));
	_p_drum_snare_inc = pitchKnobInc_10(600 - (in2 >> 8));
}
