/*
 * osc_proc_core.c
 *
 *  Created on: 28 февр. 2024 г.
 *      Author: Sx107
 */
#include <drv/rcc.h>

#include <osc_proc_core.h>
#include <drv/adc.h>
#include <drv/led.h>
#include <drv/midi.h>
#include <drv/eeprom.h>
#include <drv/buttons.h>

#include <osc_lut.h>

#include <oscs/osc_morph.h>
#include <oscs/osc_pwm.h>
#include <oscs/osc_add1.h>
#include <oscs/osc_add2.h>
#include <oscs/osc_fm.h>
#include <oscs/osc_formant.h>
#include <oscs/osc_noise.h>
#include <oscs/osc_sh.h>

#include <procs/proc_vca.h>
#include <procs/proc_vcf.h>
#include <procs/proc_att.h>
#include <procs/proc_lfo.h>
#include <procs/proc_drum.h>
#include <procs/proc_metalpipe.h>

static volatile uint8_t enable_metalpipe = 0;

volatile uint8_t osc_mode = 0;
const uint32_t 	osc_engines_color[8] =	{0x0000FF,			0x0030FF,		0x00FFFF,				0x00FF00,				0xFF9000,		0xFF0000,			0xFF00FF, 		0xFFFFFF};
const osc_engine_proc osc_engines_proc[8] = {&osc_morph_proc, &osc_pwm_proc, &osc_add1_proc, &osc_add2_proc, &osc_fm_proc, &osc_formant_proc, &osc_sh_proc, &osc_noise_proc};

volatile uint8_t proc_mode = 1;
const uint32_t 	proc_engines_color[10] =	{0x0000FF, 0x0000FF, 0x0000FF, 0x0030FF, 0x00FFFF, 0x00FF00, 0xFF9000, 0xFF0000, 0xFF00FF, 0xFFFFFF};
																																																			// 8 PROC_VCO is in the IRQ handler. Because pitch.
const proc_engine_proc proc_engines_proc[10] = 		{&proc_vcf_proc,	&proc_vcf_proc, 	&proc_vcf_proc,		&proc_vcf_proc,		&proc_vcf_proc, 	&proc_vca_proc,	&proc_att_proc, &proc_lfo_proc, 	0, 													&proc_drum_proc};
const proc_engine_update proc_engines_update[10] = 	{&proc_vcf_update,	&proc_vcf_update,	&proc_vcf_update, 	&proc_vcf_update, 	&proc_vcf_update,	0,				0,				0, 					0, 													&proc_drum_update};

volatile uint8_t pitch_mode = 0; // 0 : Knob, 1 : Midi
volatile uint8_t knob_range = 1 ; // Knob range: 0 : +-2 Semitones in midi mode, +-12 Semitones otherwise, 1 : +- 12 Semitones in both modes. Pitch input range: 0 : +- 2 Semitones, 1: +- 12 Semitones
volatile uint32_t midi_ph_inc = 70000;


volatile uint8_t osc_save_eeprom;
volatile uint8_t proc_save_eeprom;

void osc_led_update() {
	led_clear(LED_VCO_PM_0);
	led_clear(LED_VCO_PM_1);
	led_set(LED_VCO_PM_0 + pitch_mode, knob_range ? 0x00FF00 : 0x0000FF, LED_SEL_BRIGHTNESS);
}

void proc_led_update() {
	for(uint8_t i = 0; i < 10; i++) {led_clear(LED_PROC_0 + i);}
	led_set(LED_PROC_KNOB, proc_engines_color[proc_mode], LED_BRIGHTNESS);
	if(proc_mode < 5) {led_set(LED_PROC_0 + proc_mode, proc_engines_color[proc_mode], LED_SEL_BRIGHTNESS);}
	else {led_set(LED_PROC_9 + (9 - proc_mode), proc_engines_color[proc_mode], LED_SEL_BRIGHTNESS);}

}


void osc_proc_init() {


	_delayMs(10);

	if(BUT_PROC) {
		enable_metalpipe = 1; // Hehe
	}

	// DAC output
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	GPIOA->MODER |= (3 << GPIO_MODER_MODER4_Pos) | (3 << GPIO_MODER_MODER5_Pos);



	// DAC
	RCC->APB1ENR |= RCC_APB1ENR_DAC1EN;
	DAC1->CR |= (0b010 << DAC_CR_TSEL1_Pos) | (0b010 << DAC_CR_TSEL2_Pos) | DAC_CR_TEN1 | DAC_CR_TEN2;
	DAC1->CR |= DAC_CR_EN1 | DAC_CR_EN2;
	DAC1->DHR12RD = (2048 << 16) | 2048;

	// TIM17 Sub PWM?
	// Sub output (Debug output)
	// MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER7_Msk, (0b01) << GPIO_MODER_MODER7_Pos);

	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER7_Msk, (0b10) << GPIO_MODER_MODER7_Pos);
	MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFRL7_Msk, GPIO_AF1_TIM17 << GPIO_AFRL_AFRL7_Pos);
	RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
	TIM17->PSC = 0;
	TIM17->ARR = 255;
	TIM17->CCMR1 |= (0b0110 << TIM_CCMR1_OC1M_Pos);
	TIM17->CCER |= TIM_CCER_CC1E;
	TIM17->CR1 |= TIM_CR1_ARPE;
	TIM17->BDTR |= TIM_BDTR_MOE;
	TIM17->CR1 |= TIM_CR1_CEN;

	// TIM7 Main
	NVIC_SetPriority(TIM7_IRQn,  NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 3)); // 1, 3
	NVIC_EnableIRQ(TIM7_IRQn);
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
	TIM7->PSC = 0;
	TIM7->ARR = 2047;
	TIM7->CR1 |= TIM_CR1_ARPE;
	TIM7->DIER |= TIM_DIER_UIE;
	TIM7->CR2 |= 0b010 << TIM_CR2_MMS_Pos;
	__DSB();
	TIM7->CR1 |= TIM_CR1_CEN;

	// TIM6 Update
	NVIC_SetPriority(TIM6_DAC_IRQn,  NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 0)); // 2, 0
	NVIC_EnableIRQ(TIM6_DAC_IRQn);
	RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
	TIM6->PSC = 0;
	TIM6->ARR = 36000;
	TIM6->CR1 |= TIM_CR1_ARPE;
	TIM6->DIER |= TIM_DIER_UIE;
	__DSB();
	TIM6->CR1 |= TIM_CR1_CEN;

	// Other
	osc_mode = adc_get_value(ADC_VCO_MODE) >> 7;

	// EEPROM load
	uint8_t dat = eeprom_read8(EEPROM_OSC_PM);
	if(dat == 0xFF) {dat = 0;}
	pitch_mode = dat & 0b1;
	knob_range = (dat >> 1) & 0b1;

	dat = eeprom_read8(EEPROM_PROC_MODE);
	if(dat == 0xFF) {dat = 0;}

	proc_mode = dat;
	osc_led_update();
	proc_led_update();
}


void osc_button_proc() {
	static uint8_t counter = 0;
	static uint8_t prev_but = 0;
	static uint16_t but_timer = 0;
	static uint16_t but_debounce = 50;
	static uint8_t button_done = 0;

	counter += 1;
	if((counter & 0b111) != 0) {return;}

	if(BUT_VCO) {
		if (but_timer < 100) {
			but_timer += 1;
		} else if(but_timer == 100) {
			but_timer += 1;
			knob_range = 1 - knob_range;
			button_done = 1;
			osc_led_update();
			osc_save_eeprom = 1;
		}
	} else {
		but_timer = 0;
	}

	if (!BUT_VCO && prev_but) {
		if(but_debounce == 0) {
			if(!button_done) {
				pitch_mode = 1 - pitch_mode;
				osc_led_update();
				osc_save_eeprom = 1;
			}
			button_done = 0;
		}
		but_debounce = 3;

	}

	if(but_debounce > 0) {but_debounce -= 1;}

	prev_but = BUT_VCO;
}

void proc_button_proc() {
	static uint8_t counter = 0;
	static uint8_t prev_but = 0;
	static uint16_t but_debounce = 50;
	static uint16_t but_timer = 0;
	static uint8_t button_done = 0;

	counter += 1;
	if((counter & 0b111) != 0) {return;}

	if(BUT_PROC) {
		if (but_timer < 100) {
			but_timer += 1;
		} else if(but_timer == 100) {
			but_timer += 1;
			proc_mode -= 1;
			if(proc_mode > 9) {proc_mode = 9;}
			button_done = 1;
			proc_led_update();
			proc_save_eeprom = 1;
		}
	} else {
		but_timer = 0;
	}

	if (!BUT_PROC && prev_but) {
		if(but_debounce == 0) {
			if(!button_done) {
				proc_mode += 1;
				if(proc_mode >= 10) {proc_mode = 0;}
				proc_led_update();
				proc_save_eeprom = 1;
			}
			button_done = 0;
		}
		but_debounce = 5;

	}

	if(but_debounce > 0) {but_debounce -= 1;}

	prev_but = BUT_PROC;
}

void osc_proc_update() {
	if (osc_save_eeprom) {
		uint8_t dat = pitch_mode | (knob_range << 1);
		eeprom_wait_available(100);
		eeprom_write8(EEPROM_OSC_PM, dat);
		osc_save_eeprom = 0;
	}
	if (proc_save_eeprom) {
		uint8_t dat = proc_mode;
		eeprom_wait_available(100);
		eeprom_write8(EEPROM_PROC_MODE, dat);
		proc_save_eeprom = 0;
	}
}

void TIM6_DAC_IRQHandler() {
	osc_button_proc();
	proc_button_proc();

	if(proc_engines_update[proc_mode] != 0) {
		(*proc_engines_update[proc_mode])(proc_mode, adc_get_value(ADC_PROC_KNOB), ADC4->DR, ADC3->DR);
	}


	TIM6->SR &= ~TIM_SR_UIF;
}

void TIM7_IRQHandler() {
	static uint8_t cnt = 0;

	static int64_t pitch_inc = 0;
	static int64_t pitch_inc_glide_target = 0;
	static int64_t pitch_midi_inc = 0;

	static uint32_t proc_vco_phase = 0;
	static uint32_t pitch_inc_proc = 0;
	static uint32_t sub_phase = 0;
	static uint32_t sub2_phase = 0;

	uint16_t vco_output = 2048;
	uint32_t proc_output = 2048;

	//GPIOA->BSRR |= GPIO_BSRR_BS_7;

	//static int16_t om_prev = 100;
	const int16_t om_hyst = 20;
	int16_t om = adc_get_value(ADC_VCO_MODE);
	int16_t om_8 = om >> 7;
	if((om <= om_hyst) ||
			(((om & 127) >  om_hyst) && ((om & 127) < 127 - om_hyst)) ||
			(om >= 1024 - om_hyst)) {
		osc_mode = om_8;
		//om_prev = osc_mode;
	}
	//led_set(LED_VCO_PM_0, osc_engines_color[osc_mode], LED_BRIGHTNESS);
	//led_set(LED_VCO_PM_1, osc_engines_color[osc_mode], LED_BRIGHTNESS);
	led_set(LED_VCO_M_0, osc_engines_color[osc_mode], LED_BRIGHTNESS);
	led_set(LED_VCO_M_1, osc_engines_color[osc_mode], LED_BRIGHTNESS);
	led_set(LED_VCO_M_2, osc_engines_color[osc_mode], LED_BRIGHTNESS);
	led_set(LED_VCO_M_3, osc_engines_color[osc_mode], LED_BRIGHTNESS);

	//led_set(LED_PROC_0, osc_engines_color[osc_mode], LED_BRIGHTNESS);

	if(midi_get_note() != 0xFF) {
		midi_ph_inc = midi_incs[midi_get_note()];

	}

	uint64_t midi_ph_inc_pb = midi_ph_inc;
	if (knob_range == 0) {midi_ph_inc_pb = midi_ph_inc_pb * semitoneLut_14(midi_get_pb()) >> 15;}
	else {midi_ph_inc_pb = midi_ph_inc_pb * octLut_14(midi_get_pb()) >> 15;}


	int32_t adc_pitch = adc_get_value(ADC_VCO_PITCH);
	int32_t adc_pmod = adc_get_value(ADC_VCO_PMOD);

	int16_t pval = (adc_pitch + adc_pmod) >> 1;
	if(pval < 0) {pval = 0;}
	if(pval > 1023) {pval = 1023;}


	if (pitch_mode) {
		if((cnt & 0b1111) == 0) {
			pitch_inc_glide_target = midi_ph_inc_pb;
			if (midi_get_cc(86) >= 64) {pitch_inc_glide_target <<= 1;}

			if(midi_get_note_velocity() > 30) {
				pitch_midi_inc = pitch_inc_glide_target;
			} else {
				int64_t diff = (pitch_inc_glide_target - pitch_midi_inc) >> (6 + (midi_get_cc(84) >> 5));
				if(diff == 0) {pitch_midi_inc = pitch_inc_glide_target;}
				else {pitch_midi_inc += diff;}
			}

			if(knob_range == 0) {
				pitch_inc = (pitch_midi_inc * semitoneLut_10(adc_pitch)) >> 15;
				pitch_inc = (pitch_inc * semitoneLut_10(adc_pmod)) >> 15;
			} else {
				pitch_inc = (pitch_midi_inc * octLut_10(adc_pitch)) >> 15;
				pitch_inc = (pitch_inc * octLut_10(adc_pmod)) >> 15;
			}

			if (proc_mode == 8) {
				pitch_inc_proc = (pitch_midi_inc * octLut_10(adc_get_value(ADC_PROC_KNOB))) >> 15;
			}
		}
	} else {
		cnt = 0;

		if (knob_range == 0) {
			pitch_inc = pitchKnobInc_10(pval);

		} else {
			pitch_inc = pitchKnobInc_volca_11((adc_pitch >> 1) + adc_pmod);
		}
		//if(proc_mode == 8) {pitch_inc_proc = pitch_inc;}
		if(proc_mode == 8) {pitch_inc_proc = pitchKnobInc_10(adc_get_value(ADC_PROC_KNOB));}
	}


	sub_phase += (pitch_inc * 255) >> 8;
	//GPIOA->BSRR |= ((sub_phase >> (22 + (midi_get_cc(83) == 0 ? 0 : 1))) & 0b1) ? GPIO_BSRR_BS_7 : GPIO_BSRR_BR_7;

	if (midi_get_cc(82) <= 42) {TIM17->CCR1 = ((sub_phase >> (22 + (midi_get_cc(83) == 0 ? 0 : 1))) & 0b1) ? 255 : 0;} // Square
	else if (midi_get_cc(82) <= 84) {TIM17->CCR1 = ((sub_phase >> (15 + (midi_get_cc(83) == 0 ? 0 : 1))) & 0xFF);} // Saw
	else {
		// Triangle
		uint16_t v = sub_phase >> (14 + (midi_get_cc(83) == 0 ? 0 : 1));
		if (v & (1 << 8)) {TIM17->CCR1 = v & 0xFF;}
		else {TIM17->CCR1 = 255 - (v & 0xFF);}
	}


	sub2_phase += (pitch_inc * 254) >> 8;
	uint16_t sub2_v = ((sub2_phase >> 22) & 0b1) ? 4095 : 0;

	if(osc_engines_proc[osc_mode] != 0) {
		vco_output = (*osc_engines_proc[osc_mode])(pitch_inc, pval, adc_get_value(ADC_VCO_WF), adc_get_value(ADC_VCO_TIMBRE));
		if (midi_get_cc(85) > 42) {
			if (midi_get_cc(85) > 84) {
				vco_output = (vco_output + sub2_v) >> 1;
			} else {
				vco_output = (vco_output * 3 + sub2_v) >> 2;
			}
		}
	}

	if (proc_mode == 9 && enable_metalpipe) {
		proc_output = proc_metalpipe_proc(proc_mode, adc_get_value(ADC_PROC_KNOB), ADC4->DR, ADC3->DR);
	}
	else if(proc_engines_proc[proc_mode] != 0) {
		proc_output = (*proc_engines_proc[proc_mode])(proc_mode, adc_get_value(ADC_PROC_KNOB), ADC4->DR, ADC3->DR);
	} else if (proc_mode == 8) {
		proc_vco_phase += pitch_inc_proc;
		proc_output = sine_12((proc_vco_phase >> 10) & 4095) >> 4;
	}
	DAC1->DHR12RD = (vco_output << 16) | proc_output;

	cnt++;
	TIM7->SR &= ~TIM_SR_UIF;
}
