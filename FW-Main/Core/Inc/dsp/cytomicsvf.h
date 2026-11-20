#ifndef INC_DSP_CYTOMICSVF_H_
#define INC_DSP_CYTOMICSVF_H_

typedef struct {
	float g, k, a1, a2, a3;
	float m0, m1, m2;
	float ic1eq, ic2eq;
} _cytomicsvf_t;

void cytomicsvf_init(_cytomicsvf_t* filter);

float cytomicsvf_so(_cytomicsvf_t* filter, const float x) __attribute__((section (".ccmram")));

void cytomicsvf_set_coeff(_cytomicsvf_t* f, const float freq, const float q) __attribute__((section (".ccmram")));
void cytomicsvf_copy_coeff(_cytomicsvf_t* from, _cytomicsvf_t* to);

void cytomicsvf_lpf2(_cytomicsvf_t* filter) __attribute__((section (".ccmram")));
void cytomicsvf_hpf2(_cytomicsvf_t* filter) __attribute__((section (".ccmram")));
void cytomicsvf_bpf2(_cytomicsvf_t* filter) __attribute__((section (".ccmram")));
void cytomicsvf_npf2(_cytomicsvf_t* filter);
void cytomicsvf_apf2(_cytomicsvf_t* filter);

void cytomicsvf_morph(_cytomicsvf_t* filter, const float m);

#endif /* INC_DSP_CYTOMICSVF_H_ */
