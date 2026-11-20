/*
 * proc_drum.h
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */

#ifndef INC_PROCS_PROC_DRUM_H_
#define INC_PROCS_PROC_DRUM_H_

#include <main.h>

uint16_t proc_drum_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) __attribute__((section (".ccmram")));
void proc_drum_update(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2);

#endif /* INC_PROCS_PROC_DRUM_H_ */
