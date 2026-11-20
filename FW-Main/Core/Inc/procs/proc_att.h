/*
 * proc_att.h
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */

#ifndef INC_PROCS_PROC_ATT_H_
#define INC_PROCS_PROC_ATT_H_

#include <main.h>

uint16_t proc_att_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) __attribute__((section (".ccmram")));

#endif /* INC_PROCS_PROC_ATT_H_ */
