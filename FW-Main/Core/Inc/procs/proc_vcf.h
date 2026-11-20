/*
 * proc_vcf.h
 *
 *  Created on: 4 мар. 2024 г.
 *      Author: Sx107
 */

#ifndef INC_PROCS_PROC_VCF_H_
#define INC_PROCS_PROC_VCF_H_

#include <main.h>

#define MODEL_BIQUAD 0
#define MODEL_CYTOMIC 1
#define MODEL_OTASVF 2

#define FILTER_MODEL MODEL_CYTOMIC


uint16_t proc_vcf_proc(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2) __attribute__((section (".ccmram")));
void proc_vcf_update(uint8_t mode, uint16_t knob, uint16_t in1, uint16_t in2)/* __attribute__((section (".ccmram"))) */;

#endif /* INC_PROCS_PROC_VCF_H_ */
