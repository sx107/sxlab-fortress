/*
 * proc_vcf.c
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */
#include <procs/proc_vcf.h>
#include <dsp/dsp.h>
#include <osc_lut.h>
#include <drv/midi.h>

#ifndef FILTER_MODEL
	#error Set a filter model!
#endif

#if FILTER_MODEL == MODEL_BIQUAD
	_biquad_t _vcf_filter;
#elif FILTER_MODEL == MODEL_CYTOMIC
	_cytomicsvf_t vcf_ct_filter;
	_biquad_t vcf_hplow_filter;
#elif FILTER_MODEL == MODEL_OTASVF
	_otasvf_t vcf_ota_filter;
#endif



float _vcf_gain = 1.0;

uint16_t proc_vcf_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {
	static int32_t output = 0;
	static uint8_t cnt = 0;

	if((cnt & 0x01) == 0) {
		#if FILTER_MODEL == MODEL_BIQUAD
			float v = (in1 - 2048) * (1.0 / 2048) * 0.15;
			v = biquad_so(&_vcf_filter, v);

			v *= _vcf_gain;
			v += 1;
			v *= 32768;

			if(v < 0) {v = 0;}
			if (v > 65535) {v = 65535;}

			output = hardclip_lut_16(v) >> 4;

		#elif FILTER_MODEL == MODEL_CYTOMIC

			//float v = (in1 - 2048.0) * (1.0 / 2048.0) * 0.2;



			float v = in1 - 2048;
			v = v*2 + 32768;
			if(v < 0) {v = 0;}
			else if (v > 65535) {v = 65535;}
			v = (hardclip_lut_16(v) - 32768) * (1.0 / 32768) * 0.1;
			v = -cytomicsvf_so(&vcf_ct_filter, v);
			v = biquad_so(&vcf_hplow_filter, v);
			v += 1;
			v *= 32768;
			if(v < 0) {v = 0;}
			else if (v > 65535) {v = 65535;}

			output = hardclip_lut_16(v) >> 4;


		#elif FILTER_MODEL == MODEL_OTASVF

			float v = (in1 - 2048) * (1.0 / 2048) * 0.7;
			v = otasvf_so(&vcf_ota_filter, v);
			v += 1;
			v *= 32768;
			if(v < 0) {v = 0;}
			else if (v > 65535) {v = 65535;}

			output = ((uint16_t)v) >> 4;
		#endif
	}

	cnt++;

	return output;
}


void proc_vcf_update(uint8_t mode, volatile uint16_t knob, uint16_t in1, volatile uint16_t in2) {
	static float prev_freq = 0;
	static uint8_t ft = 1;
	if(ft) {
		#if FILTER_MODEL == MODEL_BIQUAD
			biquad_init(&_vcf_filter);
		#elif FILTER_MODEL == MODEL_CYTOMIC
			cytomicsvf_init(&vcf_ct_filter);
			biquad_init(&vcf_hplow_filter);
			biquad_hpf2(&vcf_hplow_filter, biquad_k(10), Q0);
		#elif FILTER_MODEL == MODEL_OTASVF
			otasvf_init(&vcf_ota_filter);
		#endif
		ft = 0;
	}



	float filt_freq = knob * (7500.0 / 1024.0);
	filt_freq += ((float)in2 - 2048) * (4000.0 / 2048.0);
	if(filt_freq < 0) {filt_freq = 0;}

	if(filt_freq < 10) {filt_freq = 10.0;}
	if(filt_freq > 7500) {filt_freq = 7500.0;}

	float final_freq = filt_freq * 0.2 + prev_freq * 0.8;
	prev_freq = final_freq;

	float Q_mod = (float)midi_get_cc(75) * (1.0f / 127.0f);
	Q_mod *= Q_mod;


	float Q = Q0;
	if (mode == 0) {Q = Q0*(1.0f+ Q_mod * 19.0f);}
	else if (mode == 1) {Q = Q0*(7.0f + Q_mod * 23.0f);}
	else if (mode == 2) {Q = Q0*(30.0f + Q_mod * 20.0f);}
	else if (mode == 3) { Q = Q0*(2.0f + Q_mod * 18.0f); }
	else if (mode == 4) { Q = Q0*(3.0f + Q_mod * 17.0f); }


	/*
	float Q = Q0;
	if (mode == 0) {Q = Q0;}
	else if (mode == 1) {Q = Q0*7;}
	else if (mode == 2) {Q = Q0*30;}
	else if (mode == 3) { Q = Q0*2; }
	else if (mode == 4) { Q = Q0*3; }
	 */

	#if FILTER_MODEL == MODEL_BIQUAD
		if(mode == 0)		{biquad_lpf2(&_vcf_filter, biquad_k(final_freq), Q0);}
		else if (mode == 1)	{biquad_lpf2(&_vcf_filter, biquad_k(final_freq), Q0*3);}
		else if (mode == 2)	{biquad_lpf2(&_vcf_filter, biquad_k(final_freq), Q0*10);}
		else if (mode == 3)	{biquad_bpf2(&_vcf_filter, biquad_k(final_freq), Q0);}
		else if (mode == 4)	{biquad_hpf2(&_vcf_filter, biquad_k(final_freq), Q0);}
	#endif

	#if FILTER_MODEL == MODEL_CYTOMIC

		float qk = 1;
		if (final_freq < 60) {qk = (final_freq - 10) * (1.0 / 50.0);}
		if (qk < 0) {qk = 0;}
		Q = Q0 + (Q - Q0) * qk;

		if (mode <= 2) {cytomicsvf_lpf2(&vcf_ct_filter);}
		else if (mode == 3) {cytomicsvf_bpf2(&vcf_ct_filter);}
		else if (mode == 4) {cytomicsvf_hpf2(&vcf_ct_filter);}

		cytomicsvf_set_coeff(&vcf_ct_filter, biquad_k_log(final_freq), Q);
	#endif

	#if FILTER_MODEL == MODEL_OTASVF
		if (mode <= 2) {otasvf_lpf2(&vcf_ota_filter);}
		else if (mode == 3) {otasvf_bpf2(&vcf_ota_filter);}
		else if (mode == 4) {otasvf_hpf2(&vcf_ota_filter);}

		otasvf_set_coeff(&vcf_ota_filter, biquad_k_log(final_freq), Q);
	#endif

}
