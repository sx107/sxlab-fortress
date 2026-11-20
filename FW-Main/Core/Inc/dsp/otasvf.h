#ifndef INC_DSP_OTASVF_H_
#define INC_DSP_OTASVF_H_

typedef struct {
	float zi;
	float s0, s1;
	float m0, m1, m2;
	float r, f;
} _otasvf_t;

void otasvf_init(_otasvf_t* filter);

float otasvf_so(_otasvf_t* filter, const float x) __attribute__((section (".ccmram")));

void otasvf_set_coeff(_otasvf_t* f, const float freq, const float q) __attribute__((section (".ccmram")));
void otasvf_copy_coeff(_otasvf_t* from, _otasvf_t* to);

void otasvf_lpf2(_otasvf_t* filter) __attribute__((section (".ccmram")));
void otasvf_hpf2(_otasvf_t* filter) __attribute__((section (".ccmram")));
void otasvf_bpf2(_otasvf_t* filter) __attribute__((section (".ccmram")));
void otasvf_npf2(_otasvf_t* filter);
void otasvf_apf2(_otasvf_t* filter);

void otasvf_morph(_otasvf_t* filter, const float m);

#endif /* INC_DSP_OTASVF_H_ */
