#include "main.h"
#include "dsp/dsp.h"
#include "dsp/otasvf.h"

static inline float tanhXdX(const float x) {
	return (15+x*x) / (15 + 6*x*x);
}


void otasvf_init(_otasvf_t* filter) {
	filter->s0 = 0;
	filter->s1 = 0;
	filter->m0 = 0;
	filter->m1 = 0;
	filter->m2 = 0;
	filter->zi = 0;
}

/*
float otasvf_so(_otasvf_t* filter, const float x) {
	float ih = 0.5 * (x + filter->zi);
	filter->zi = x;

	float t0 = tanhXdX(ih-2*filter->r*filter->s0 - filter->s1);
	float t1 = tanhXdX(filter->s0);

	float g0 = 1.0 / (1.0 + filter->f * t0);
	float g1 = 1.0 / (1.0 + filter->f * t1);

	float xx = (-g1*filter->s1-g0*filter->s0*(2*filter->r+filter->f*g1*t1)+x) / (1 + 2*filter->f*g0*filter->r*t0+filter->f*filter->f*g0*g1*t0*t1);
	float y0 = g0*(filter->s0 + filter->f * t0 * xx);
	float y1 = g1*(filter->s1 + filter->f * t1 * y0);

	filter->s0 += 2 * filter->f * t0 * (xx);
	filter->s1 += 2 * filter->f * t1 * (y0);

	return filter->m0 * xx + filter->m1 * y0 + filter->m2 * y1;
}
*/

float otasvf_so(_otasvf_t* f, const float x) {
	//volatile uint32_t start = DWT->CYCCNT;

	const float ih = 0.5 * (x + f->zi);
	f->zi = x;

	const float a = ih - 2.0f * f->r * f->s0 - f->s1;
	const float b = f->s0;
	const float t0 = tanhXdX(a);
	const float t1 = tanhXdX(b);


	const float g0 = 1.0f / (1.0f + f->f * t0);
	const float g1 = 1.0f / (1.0f + f->f * t1);

	const float r2 = 2.0f * f->r;
	const float g0f = g0 * f->f;
	const float g1f = g1 * f->f;
	const float r2_g0f = r2 * g0f;

	const float num = -g1*f->s1 - g0*f->s0 * (r2 + g1f*t1) + x;
	const float denom = 1.0f + r2_g0f * t0 + g0f * g1f * t0 * t1;

	const float xx = num / denom;
	const float y0 = g0 * (f->s0 + f->f * t0 * xx);
	const float y1 = g1 * (f->s1 + f->f * t1 * y0);

	f->s0 += 2.0f * f->f * t0 * xx;
	f->s1 += 2.0f * f->f * t1 * y0;

	const float ret = f->m0 * xx + f->m1 * y0 + f->m2 * y1;

	//volatile uint32_t end = DWT->CYCCNT;
	//__asm("NOP");


	return ret;
}

void otasvf_set_coeff(_otasvf_t* filter, const float freq, const float q) {
	filter->f = freq;
	filter->r = 1.0/q;
}
void otasvf_copy_coeff(_otasvf_t* from, _otasvf_t* to) {
	to->f = from->f;
	to->r = from->r;
}

void otasvf_lpf2(_otasvf_t* filter) {
	filter->m0 = 0;
	filter->m1 = 0;
	filter->m2 = 1;
}
void otasvf_hpf2(_otasvf_t* filter)  {
	filter->m0 = 1;
	filter->m1 = 0;
	filter->m2 = 0;
}
void otasvf_bpf2(_otasvf_t* filter)  {
	filter->m0 = 0;
	filter->m1 = 1;
	filter->m2 = 0;
}

void otasvf_morph(_otasvf_t* filter, const float m) {
	float mm = m;
	if (mm < 0.5) {
		mm *= 2;
		filter->m0 = 0;
		filter->m1 = mm;
		filter->m2 = 1.0 - mm;
	} else {
		mm = (mm - 0.5) * 2;
		filter->m0 = mm;
		filter->m1 = 1.0-mm;
		filter->m2 = 0;
	}
}
