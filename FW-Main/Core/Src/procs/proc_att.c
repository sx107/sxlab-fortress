/*
 * proc_att.c
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */

#include <procs/proc_att.h>

uint16_t proc_att_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) {
	return (in1 * knob) >> 10;
}
