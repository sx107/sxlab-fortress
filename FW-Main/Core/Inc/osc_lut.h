/*
 * osc_lut.h
 *
 *  Created on: 2 мар. 2024 г.
 *      Author: Sx107
 */

#ifndef INC_OSC_LUT_H_
#define INC_OSC_LUT_H_

#include <main.h>

uint16_t sine_clip_12(uint16_t v);
uint16_t sine_12(uint16_t v) __attribute__((section (".ccmram"))) ;

uint32_t semitoneLut_14(uint16_t v);
uint32_t semitoneLut_10(uint16_t v);
uint32_t octLut_14(uint16_t v);
uint32_t octLut_10(uint16_t v);

extern uint32_t midi_incs[110];
uint32_t pitchKnobInc_10(uint16_t v);
uint32_t pitchKnobInc_volca_11(uint16_t v);

uint16_t vca_gain_lut_12(uint16_t v);
uint16_t hardclip_lut_16(uint16_t v) __attribute__((section (".ccmram")));

#endif /* INC_OSC_LUT_H_ */
