#include "dsp/dsp.h"
#include "dsp/cytomicsvf.h"

void cytomicsvf_init(_cytomicsvf_t* f) {
	f->ic1eq = 0;
	f->ic2eq = 0;
}

float cytomicsvf_so(_cytomicsvf_t* f, const float v0) {
    float v3 = v0 - f->ic2eq;
    float v1 = f->a1 * f->ic1eq + f->a2 * v3;
    float v2 = f->ic2eq + f->a2 * f->ic1eq + f->a3 * v3;
    f->ic1eq = 2.0f * v1 - f->ic1eq;
    f->ic2eq = 2.0f * v2 - f->ic2eq;
    return f->m0 * v0 + f->m1 * v1 + f->m2 * v2;
}

void cytomicsvf_set_coeff(_cytomicsvf_t* f, const float freq, const float q) {
	f->g = freq;

	float qq = q;
	//if(qq > 50) {qq = 50;}

	f->k = 1.0 / qq;

	//if(qq > 49) {
	//	f->k = f->k * (50 - qq);
	//}

	f->a1 = 1.0 / (1.0 + f->g * (f->g + f->k));
	f->a2 = f->g * f->a1;
	f->a3 = f->g * f->a2;
}

void cytomicsvf_copy_coeff(_cytomicsvf_t* from, _cytomicsvf_t* to) {
	to->g = from->g;
	to->k = from->k;
	to->a1 = from->a1;
	to->a2 = from->a2;
	to->a3 = from->a3;
}

void cytomicsvf_lpf2(_cytomicsvf_t* filter) {
	filter->m0 = 0;
	filter->m1 = 0;
	filter->m2 = 1.0;
}
void cytomicsvf_hpf2(_cytomicsvf_t* filter) {
	filter->m0 = 1.0;
	filter->m1 = -filter->k;
	filter->m2 = -1.0;
}
void cytomicsvf_bpf2(_cytomicsvf_t* filter) {
	filter->m0 = 0;
	filter->m1 = filter->k;
	filter->m2 = 0;
}
void cytomicsvf_npf2(_cytomicsvf_t* filter) {
	filter->m0 = 1.0;
	filter->m1 = -filter->k;
	filter->m2 = 0;
}
void cytomicsvf_apf2(_cytomicsvf_t* filter) {
	filter->m0 = 1.0;
	filter->m1 = -2.0 * filter->k;
	filter->m2 = 0;
}

void cytomicsvf_morph(_cytomicsvf_t* filter, const float m) {
	float mm = m;

	float m0_l = 0;
	float m1_l = 0;
	float m2_l = 1.0;

	float m0_r = 0.0;
	float m1_r = filter->k;
	float m2_r = 0.0;

	if (mm < 0.5) {
		mm *= 2.0;
	} else {
		mm = (mm - 0.5) * 2.0;
		m0_l = m0_r;
		m1_l = m1_r;
		m2_l = m2_r;

		m0_r = 1.0;
		m1_r = -filter->k;
		m2_r = -1.0;
	}

	filter->m0 = m0_l + (m0_r - m0_l) * mm;
	filter->m1 = m1_l + (m1_r - m1_l) * mm;
	filter->m2 = m2_l + (m2_r - m2_l) * mm;
}
