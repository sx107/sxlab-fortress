/*
 * proc_metalpipe.h
 *
 *  Created on: 7 апр. 2025 г.
 *      Author: Sx107
 */

#ifndef INC_PROCS_PROC_METALPIPE_H_
#define INC_PROCS_PROC_METALPIPE_H_

#include <main.h>
#define MP_SMPSIZE 33396
extern const uint8_t sound_sample[33396];

uint16_t proc_metalpipe_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2);
void proc_metalpipe_update(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2);


#endif /* INC_PROCS_PROC_METALPIPE_H_ */
