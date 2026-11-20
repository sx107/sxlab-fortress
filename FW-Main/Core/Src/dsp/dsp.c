/*
 * dsp.c
 *
 *  Created on: Feb 9, 2024
 *      Author: Sx107
 */


#include <dsp/dsp.h>

float clamp0f(float x) {
	return x > 1.0 ? 1.0 : (x < 0.0 ? 0.0 : x);
}
float clampf(float x) {
	return x > 1.0 ? 1.0 : (x < -1.0 ? -1.0 : x);
}



float lut(const uint16_t* l, const float lut_size, const float x) {
	float f = clamp0f(x) * lut_size;
	uint16_t L_index = (uint16_t) f;

	float left = l[L_index] * U16_TO_F;
	float right = l[L_index + 1] * U16_TO_F;
	float frac = f - L_index;

	return clampf(left + (right - left) * frac);
}

float lut256(const uint16_t* l, const float x) {
	return lut(l, 256, x);
}

