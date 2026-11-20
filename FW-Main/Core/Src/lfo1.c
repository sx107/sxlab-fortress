/*
 * lfo1.c
 *
 *  Created on: Feb 25, 2024
 *      Author: Sx107
 */

#include <string.h>

#include <lfo_timer.h>
#include <lfo1.h>
#include <drv/adc.h>
#include <drv/led.h>
#include <drv/eeprom.h>
#include <drv/buttons.h>
#include <rnd.h>

// Phase is 24-bit
volatile uint32_t lfo1_phase = 0;
volatile uint8_t lfo1_prev_sq = 0;

// LFO1 Mode
volatile uint8_t lfo1_mode = 0;

// Morph filter
volatile uint32_t lfo1_prev_wf = 0;

// Freq filter
volatile uint32_t lfo1_prev_freq = 0;

// Rungler
volatile uint8_t lfo1_rungler = 0b10010101;
volatile uint32_t lfo1_rungler_init = 0xECAF;
volatile uint8_t lfo1_rungler_length = 0;

// EEPROM
volatile uint8_t lfo1_save_eeprom = 0;

// Sync
#define LFO1SYNC ((GPIOB->IDR & GPIO_IDR_15) != 0)
volatile uint8_t lfo1_prev_sync = 0;

// For the trigger mode
volatile uint32_t lfo1_trig_output = 0;
volatile uint8_t lfo1_trig_state = 0; // 0 standby, 1 rising, 2 falling

// 64
//const uint16_t lfo1_freq_lut[] = {191, 209, 229, 250, 274, 300, 329, 360, 394, 431, 472, 517, 566, 619, 678, 742, 813, 890, 974, 1066, 1167, 1278, 1399, 1532, 1677, 1836, 2009, 2200, 2408, 2637, 2886, 3160, 3459, 3787, 4146, 4539, 4969, 5440, 5955, 6520, 7138, 7814, 8554, 9365, 10253, 11224, 12288, 13452, 14727, 16122, 17650, 19323, 21154, 23158, 25353, 27755, 30385, 33265, 36417, 39868, 43646, 47782, 52309, 57266, 62693};
const uint16_t lfo1_freq_lut[] = {10,11,13,14,17,19,22,25,29,33,38,44,50,57,66,76,87,100,115,132,151,173,199,229,262,301,346,397,456,523,601,690,792,909,1044,1199,1376,1580,1814,2083,2391,2745,3151,3618,4154,4769,5475,6286,7217,8285,9512,10921,12538,14394,16526,18973,21782,25008,28711,32962,37843,43447,49880,57266, /*65746*/ 62535};
const uint16_t lfo1_ar_lut[] = {59652, 52533, 46264, 40743, 35880, 31598, 27827, 24506, 21582, 19006, 16738, 14740, 12981, 11432, 10068, 8866, 7808, 6876, 6056, 5333, 4697, 4136, 3642, 3208, 2825, 2488, 2191, 1929, 1699, 1496, 1318, 1161, 1022, 900, 793, 698, 615, 541, 477, 420, 370, 326, 287, 253, 222, 196, 172, 152, 134, 118, 104, 91, 80, 71, 62, 55, 48, 43, 38, 33, 29, 26, 23, 20, 18};

// v = 0-1023
uint16_t lfo1_inc(const uint16_t v) {
	uint32_t lut_low = lfo1_freq_lut[v >> 4];
	uint32_t lut_high = lfo1_freq_lut[(v >> 4) + 1];
	return lut_low + (((lut_high - lut_low) * (v & 0b1111)) >> 4);
}

uint16_t lfo1_ar_inc(const uint16_t v) {
	uint32_t lut_low = lfo1_ar_lut[v >> 4];
	uint32_t lut_high = lfo1_ar_lut[(v >> 4) + 1];
	return lut_low + (((lut_high - lut_low) * (v & 0b1111)) >> 4);
}

void lfo1_rungler_process() {
	lfo1_rungler = (lfo1_rungler << 1) | (((lfo1_rungler >> 7) & 0b1) ^ lfo1_rungler_length);
	uint16_t rungler = ((lfo1_rungler & 0b1) | ((lfo1_rungler >> 2) & 0b10) | ((lfo1_rungler >> 3) & 0b100)) << 7;
	LFO1RND = rungler & 1023;
}
void lfo1_rungler_rnd() {
	lfo1_rungler_init = xorshift32(lfo1_rungler_init);
	lfo1_rungler = lfo1_rungler_init & 0xFF;
	lfo1_save_eeprom = 1;
}

void lfo1_reset() {
	lfo1_phase = 0;
	lfo1_prev_sq = 0;

	lfo1_trig_output = 0;
	lfo1_trig_state = 0;

	lfo1_rungler_process();
}
void lfo1_full_reset() {
	if(lfo1_mode) {lfo1_reset();}
}

void lfo1_led_update() {
	led_clear(LED_LFO1_0); led_clear(LED_LFO1_1); led_clear(LED_LFO1_2);
	uint32_t color = 0x0000FF;
	if(lfo1_rungler_length == 1) {color = 0x00FF00;}
	led_set(LED_LFO1_0 - lfo1_mode, color, LED_SEL_BRIGHTNESS);
}

void lfo1_init() {
	// Init the SQR output
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER14_Msk, (0b01) << GPIO_MODER_MODER14_Pos);

	// Init the sync input
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER15_Msk, (0b00) << GPIO_MODER_MODER15_Pos);

	eeprom_wait_available(1000);
	lfo1_rungler_init = eeprom_read32(EEPROM_LFO1_RUNGLER);
	if(lfo1_rungler_init == 0xFFFFFFFF) {lfo1_rungler_init = 0xCAFECAFE;}
	lfo1_rungler = lfo1_rungler_init & 0xFF;

	eeprom_wait_available(1000);
	uint8_t dat = eeprom_read8(EEPROM_LFO1_DAT);
	if(dat == 0xFF) {dat = 0;}
	lfo1_mode = dat & 0b11;
	lfo1_rungler_length = (dat >> 2) & 0b1;


	lfo1_led_update();
}



void lfo1_but_process() {
	static uint8_t prev_but = 0;
	static uint16_t but_timer = 0;
	static uint16_t but_debounce = 0;
	static uint16_t blink_counter = 0;
	static uint8_t button_done = 0;

	if(LFO1BUT) {
		if(but_timer < 20000) {but_timer++;}
		else if (but_timer == 20000) {
			but_timer++;
			button_done = 1;
			lfo1_rungler_length  = 1 - lfo1_rungler_length;
			lfo1_rungler_rnd(); // will save to EEPROM
			blink_counter = 4 * 2048;
		}
	}

	if(!LFO1BUT && prev_but) {
		if (!button_done && but_debounce == 0) {
			lfo1_reset();
			lfo1_mode += 1;
			if(lfo1_mode > 2) {
				lfo1_mode = 0;
			}
			blink_counter = 0;
			lfo1_led_update();
			lfo1_save_eeprom = 1;
		}
		but_debounce = 1000;
	}
	if(but_debounce > 0) {but_debounce--;}

	if(!LFO1BUT) {
		but_timer = 0;
		button_done = 0;
	}

	prev_but = LFO1BUT;

	if(blink_counter > 1) {
		if(((blink_counter >> 11) & 0b1) == 1) {
			led_clear(LED_LFO1_0); led_clear(LED_LFO1_1); led_clear(LED_LFO1_2);
		} else {
			led_set(LED_LFO1_0, 0xFFFFFF, LED_BRIGHTNESS);
			led_set(LED_LFO1_1, 0xFFFFFF, LED_BRIGHTNESS);
			led_set(LED_LFO1_2, 0xFFFFFF, LED_BRIGHTNESS);
		}
		blink_counter--;
	} else if (blink_counter == 1) {
		lfo1_led_update();
		blink_counter--;
	}
}

static inline void lfo1_process_lfo();
static inline void lfo1_process_trig();
static inline void lfo1_process_gate();

void lfo1_process_lfo() {
	if(LFO1SYNC && !lfo1_prev_sync) {
		lfo1_reset();
	}
	lfo1_prev_sync = LFO1SYNC;

	uint16_t lfo_current_wf = adc_get_value(ADC_LFO1_WF);
	uint16_t lfoWF = (lfo1_prev_wf * 15 + lfo_current_wf)>>4;
	lfo1_prev_wf = lfoWF;

	uint16_t lfo_current_freq = adc_get_value(ADC_LFO1_SPD);
	uint16_t lfoFreq = (lfo1_prev_freq * 15 + lfo_current_freq)>>4;
	lfo1_prev_freq = lfoFreq;

	lfo1_phase += lfo1_inc(lfoFreq);
	uint16_t lfoPhaseQ = 2047 & (lfo1_phase >> (24-11));

	GPIOB->BSRR |= ((lfoPhaseQ >>10) == 0) ? GPIO_BSRR_BS_14 : GPIO_BSRR_BR_14;

	if((lfoPhaseQ >> 10) == 0 && lfo1_prev_sq == 1) {
		lfo1_rungler_process();
	}
	lfo1_prev_sq = lfoPhaseQ >> 10;

	if(lfoWF == 0) {
		LFO1OUT = (lfoPhaseQ >> 1); // Rising saw
	} else if (lfoWF < 512) {
		lfoWF = lfoWF << 2; // From 0-512 to 0-2048
		if(lfoPhaseQ < 2048-lfoWF) {
			LFO1OUT = (lfoPhaseQ) * 1023 / (2047-lfoWF);
		} else {
			LFO1OUT = 1023 - (lfoPhaseQ-(2048-lfoWF)) * 1023 / lfoWF;
		}
	} else {
		lfoWF = 2047 - ((lfoWF - 512) << 2);
		if(lfoPhaseQ == 0) {
			LFO1OUT = 1023;
		}
		else if(lfoWF < 128) {
			LFO1OUT = lfoPhaseQ < 128 ? 1023 - (lfoPhaseQ) * 8 : 0;
		}
		else if(lfoPhaseQ < lfoWF) {
			LFO1OUT = 1023 - (lfoPhaseQ) * 1023 / lfoWF; // Falling saw
		}
	}
}

void lfo1_process_trig() {
	uint16_t lfo_current_wf = adc_get_value(ADC_LFO1_WF);
	uint16_t lfoRelease = (lfo1_prev_wf * 15 + lfo_current_wf)>>4;
	lfo1_prev_wf = lfoRelease;

	uint16_t lfo_current_freq = adc_get_value(ADC_LFO1_SPD);
	uint16_t lfoAttack = (lfo1_prev_freq * 15 + lfo_current_freq)>>4;
	lfo1_prev_freq = lfoAttack;

	lfoAttack = lfo1_ar_inc(lfoAttack);
	lfoRelease = lfo1_ar_inc(lfoRelease);

	// Was here...

	if(lfo1_trig_state == 1) {
		lfo1_trig_output += lfoAttack;

		if(lfo1_trig_output >= (1 << 20) - 1) { // 2^20 - 1
			lfo1_trig_output = (1 << 20) - 1;
			lfo1_trig_state = 2;
		}
		LFO1OUT = lfo1_trig_output >> 10;
		GPIOB->BSRR |= GPIO_BSRR_BS_14;
	} else if (lfo1_trig_state == 2) {
		if(lfo1_trig_output > lfoRelease) {lfo1_trig_output -= lfoRelease;}
		else {lfo1_trig_output = 0; lfo1_trig_state = 0;}
		LFO1OUT = lfo1_trig_output >> 10;
		GPIOB->BSRR |= GPIO_BSRR_BS_14;
	} else {
		GPIOB->BSRR |= GPIO_BSRR_BR_14;
		LFO1OUT = 0;
	}

	if(LFO1SYNC && !lfo1_prev_sync) {
		lfo1_rungler_process();
		lfo1_trig_output = 0;
		lfo1_trig_state = 1;
	}
	lfo1_prev_sync = LFO1SYNC;
}



void lfo1_process_gate() {
	uint16_t lfo_current_wf = adc_get_value(ADC_LFO1_WF);
	uint16_t lfoRelease = (lfo1_prev_wf * 15 + lfo_current_wf)>>4;
	lfo1_prev_wf = lfoRelease;

	uint16_t lfo_current_freq = adc_get_value(ADC_LFO1_SPD);
	uint16_t lfoAttack = (lfo1_prev_freq * 15 + lfo_current_freq)>>4;
	lfo1_prev_freq = lfoAttack;

	lfoAttack = lfo1_ar_inc(lfoAttack);
	lfoRelease = lfo1_ar_inc(lfoRelease);

	if(LFO1SYNC && !lfo1_prev_sync) {
		lfo1_rungler_process();
	}
	lfo1_prev_sync = LFO1SYNC;

	if(LFO1SYNC) {
		lfo1_trig_output += lfoAttack;
		if(lfo1_trig_output >= (1 << 20) - 1) { // 2^20 - 1
			lfo1_trig_output = (1 << 20) - 1;
			GPIOB->BSRR |= GPIO_BSRR_BS_14;
		} else {
			GPIOB->BSRR |= GPIO_BSRR_BR_14;
		}

	} else {
		if(lfo1_trig_output > lfoRelease) {lfo1_trig_output -= lfoRelease;}
		else {lfo1_trig_output = 0; }
		GPIOB->BSRR |= GPIO_BSRR_BR_14;
	}

	LFO1OUT = lfo1_trig_output >> 10;
}

void lfo1_process() {
	lfo1_but_process();

	if(lfo1_mode == 0) {
		lfo1_process_lfo();
	} else if (lfo1_mode == 1) {
		lfo1_process_trig();
	} else {
		lfo1_process_gate();
	}
}

void lfo1_update() {
	if(lfo1_save_eeprom) {
		uint8_t dat = lfo1_mode | (lfo1_rungler_length << 2);
		uint8_t buf[5] = {dat, 0, 0, 0, 0};
		memcpy(&buf[1], &lfo1_rungler_init, 4);
		eeprom_wait_available(100);
		eeprom_write_buffer(EEPROM_LFO1_DAT, buf, 5);
		//_delayMs(6);
		lfo1_save_eeprom = 0;
	}
}




