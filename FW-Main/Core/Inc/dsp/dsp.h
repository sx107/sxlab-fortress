/*
 * dsp.h
 *
 *  Created on: Feb 9, 2024
 *      Author: Sx107
 */



#ifndef DSP_DSP_H_
#define DSP_DSP_H_

#include <dsp/otasvf.h>
#include <dsp/cytomicsvf.h>
#include <dsp/biquad.h>
#include <stdint.h>

#define FS (72000000.0 / 2048.0)
#define INVFS (1.0 / FS)
#define SAMPLES_MS 48
#define SAMPLES_S 47957

#define F_TO_S32 (2147483646.0)
#define S32_TO_F (1.0 / 2147483648.0)

#define U16_TO_F (1.0 / 65535.0)
#define U12_TO_F (1.0 / 4095.0)

#define M_PI (3.14159265358979)

float lut(const uint16_t* lut, const float lut_size, const float x);
float lut256(const uint16_t* lut, const float x);

float clamp0f(float x);
float clampf(float x);

#endif /* DSP_DSP_H_ */
