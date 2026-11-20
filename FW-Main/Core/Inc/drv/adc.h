/*
 * adc.h
 *
 *  Created on: Feb 24, 2024
 *      Author: Sx107
 */

#ifndef ADC_H_
#define ADC_H_

#include <main.h>

#define ADC_SMP_TIME 0b111

extern volatile uint16_t adcData1[5*4];
extern volatile uint16_t adcData2[5*4];
volatile uint16_t adcData[10];

#define ADC_VCO_TIMBRE	0
#define ADC_LFO1_SPD	1
#define ADC_LFO1_WF		2
#define ADC_LFO2_SPD	3
#define ADC_LFO2_WF		4


#define ADC_PROC_KNOB 	5
#define ADC_VCO_MODE	6
#define ADC_VCO_PITCH	7
#define ADC_VCO_PMOD	8
#define ADC_VCO_WF		9



void adc_init();
uint16_t adc_get_value(uint8_t v);
uint16_t adc_raw_value(uint8_t v);

#endif /* ADC_H_ */
