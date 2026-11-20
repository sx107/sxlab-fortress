#include <stdint.h>

#ifndef DSP_BIQUAD_H_
#define DSP_BIQUAD_H_

#define Q0 0.707107


typedef struct {
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;

	float mZ1, mZ2; // States
} _biquad_t;

void biquad_init(_biquad_t* filter);

float biquad_so(_biquad_t* filter, const float x) __attribute__((section (".ccmram")));
float biquad_fo(_biquad_t* filter, const float x) __attribute__((section (".ccmram")));

float biquad_k(float freq);
float biquad_k_log(float freq);

void biquad_lpf1(_biquad_t* filter, const float k);
void biquad_hpf1(_biquad_t* filter, const float k);

void biquad_apf1(_biquad_t* filter, const float k);

void biquad_lpf2(_biquad_t* filter, const float k, const float q) __attribute__((section (".ccmram")));
void biquad_hpf2(_biquad_t* filter, const float k, const float q) __attribute__((section (".ccmram")));
void biquad_bpf2(_biquad_t* filter, const float k, const float q) __attribute__((section (".ccmram")));
void biquad_npf2(_biquad_t* filter, const float k, const float q);
void biquad_dc2(_biquad_t* filter, const float pole);

void biquad_apf2(_biquad_t* filter, const float k, const float q);

#endif /* DSP_BIQUAD_H_ */
