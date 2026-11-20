/*
 * osc_am.h
 *
 *  Created on: 3 мар. 2024 г.
 *      Author: Sx107
 */

#ifndef INC_OSCS_OSC_SH_H_
#define INC_OSCS_OSC_SH_H_

#include <main.h>

uint16_t osc_sh_proc(uint32_t phase_inc, uint16_t pitch, uint16_t wf, uint16_t timbre) __attribute__((section (".ccmram")));

#endif /* INC_OSCS_OSC_SH_H_ */
