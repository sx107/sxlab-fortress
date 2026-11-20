/*
 * lfo2.c
 *
 *  Created on: Feb 24, 2024
 *      Author: Sx107
 */

#include <string.h>

#include <lfo_timer.h>
#include <lfo2.h>
#include <drv/adc.h>
#include <drv/led.h>
#include <drv/eeprom.h>
#include <drv/buttons.h>
#include <rnd.h>


// Phase is 24-bit
volatile uint32_t lfo2_phase = 0;
volatile uint8_t lfo2_prev_sq = 0;

// Reset counter
volatile uint8_t lfo2_mode = 0;
volatile uint8_t lfo2_sync_cnt = 0;

// Morph filter
volatile uint32_t lfo2_prev_wf = 0;

// Freq filter
volatile uint32_t lfo2_prev_freq = 0;

// Rungler
volatile uint8_t lfo2_rungler = 0b10010101;
volatile uint32_t lfo2_rungler_init = 0xECAF;
volatile uint8_t lfo2_rungler_length = 0;

// EEPROM
volatile uint8_t lfo2_save_eeprom = 0;

// Sync
#define LFO2SYNC ((GPIOA->IDR & GPIO_IDR_10) != 0)
volatile uint8_t lfo2_prev_sync = 0;

// 64
//const uint16_t lfo2_freq_lut[] = {191, 209, 229, 250, 274, 300, 329, 360, 394, 431, 472, 517, 566, 619, 678, 742, 813, 890, 974, 1066, 1167, 1278, 1399, 1532, 1677, 1836, 2009, 2200, 2408, 2637, 2886, 3160, 3459, 3787, 4146, 4539, 4969, 5440, 5955, 6520, 7138, 7814, 8554, 9365, 10253, 11224, 12288, 13452, 14727, 16122, 17650, 19323, 21154, 23158, 25353, 27755, 30385, 33265, 36417, 39868, 43646, 47782, 52309, 57266, 62693};
const uint16_t lfo2_freq_lut[] = {10,11,13,14,17,19,22,25,29,33,38,44,50,57,66,76,87,100,115,132,151,173,199,229,262,301,346,397,456,523,601,690,792,909,1044,1199,1376,1580,1814,2083,2391,2745,3151,3618,4154,4769,5475,6286,7217,8285,9512,10921,12538,14394,16526,18973,21782,25008,28711,32962,37843,43447,49880,57266, /*65746*/ 62536};

// v = 0-1023
uint16_t lfo2_inc(const uint16_t v) {
	uint32_t lut_low = lfo2_freq_lut[v >> 4];
	uint32_t lut_high = lfo2_freq_lut[(v >> 4) + 1];
	return lut_low + (((lut_high - lut_low) * (v & 0b1111)) >> 4);
}

void lfo2_rungler_process() {
	lfo2_rungler = (lfo2_rungler << 1) | (((lfo2_rungler >> 7) & 0b1) ^ lfo2_rungler_length);
	uint16_t rungler = ((lfo2_rungler & 0b1) | ((lfo2_rungler >> 2) & 0b10) | ((lfo2_rungler >> 3) & 0b100)) << 7;
	//uint16_t rungler = lfo2_rungler << 2;
	LFO2RND = rungler & 1023;
}
void lfo2_rungler_rnd() {
	lfo2_rungler_init = xorshift32(lfo2_rungler_init);
	lfo2_rungler = lfo2_rungler_init & 0xFF;
	lfo2_save_eeprom = 1;
}

void lfo2_reset() {
	lfo2_phase = 0;
	lfo2_prev_sq = 0;
	lfo2_rungler_process();
}
void lfo2_full_reset() {
	lfo2_reset();
	lfo2_sync_cnt = 0;
}
void lfo2_midi_reset() {
	lfo2_sync_cnt = 0;
}

void lfo2_led_update() {
	led_clear(LED_LFO2_0); led_clear(LED_LFO2_1); led_clear(LED_LFO2_2);
	uint32_t color = 0x0000FF;
	if(lfo2_rungler_length == 1) {color = 0x00FF00;}
	led_set(LED_LFO2_0 - lfo2_mode, color, LED_SEL_BRIGHTNESS);
}

void lfo2_init() {
	// Init the SQR output
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER9_Msk, (0b01) << GPIO_MODER_MODER9_Pos);

	// Init the button input
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER10_Msk, (0b00) << GPIO_MODER_MODER10_Pos);

	eeprom_wait_available(1000);
	lfo2_rungler_init = eeprom_read32(EEPROM_LFO2_RUNGLER);
	if(lfo2_rungler_init == 0xFFFFFFFF) {lfo2_rungler_init = 0xFACEFACE;}
	lfo2_rungler = lfo2_rungler_init & 0xFF;

	eeprom_wait_available(1000);
	uint8_t dat = eeprom_read8(EEPROM_LFO2_DAT);
	if(dat == 0xFF) {dat = 0;}
	lfo2_mode = dat & 0b11;
	lfo2_rungler_length = (dat >> 2) & 0b1;


	lfo2_led_update();
}

void lfo2_but_process() {
	static uint8_t prev_but = 0;
	static uint16_t but_timer = 0;
	static uint16_t but_debounce = 0;
	static uint16_t blink_counter = 0;
	static uint8_t button_done = 0;

	if(LFO2BUT) {
		if(but_timer < 20000) {but_timer++;}
		else if (but_timer == 20000) {
			but_timer++;
			button_done = 1;
			lfo2_rungler_length  = 1 - lfo2_rungler_length;
			lfo2_rungler_rnd(); // will save to EEPROM
			blink_counter = 4 * 2048;
		}
	}

	if(!LFO2BUT && prev_but) {
		if (!button_done && but_debounce == 0) {
			lfo2_mode += 1;
			if(lfo2_mode > 2) {
				lfo2_mode = 0;
			}
			blink_counter = 0;
			lfo2_led_update();
			lfo2_save_eeprom = 1;
		}
		but_debounce = 1000;
	}
	if(but_debounce > 0) {but_debounce--;}

	if(!LFO2BUT) {
		but_timer = 0;
		button_done = 0;
	}

	prev_but = LFO2BUT;

	if(blink_counter > 1) {
		if(((blink_counter >> 11) & 0b1) == 1) {
			led_clear(LED_LFO2_0); led_clear(LED_LFO2_1); led_clear(LED_LFO2_2);
		} else {
			led_set(LED_LFO2_0, 0xFFFFFF, LED_BRIGHTNESS);
			led_set(LED_LFO2_1, 0xFFFFFF, LED_BRIGHTNESS);
			led_set(LED_LFO2_2, 0xFFFFFF, LED_BRIGHTNESS);
		}
		blink_counter--;
	} else if (blink_counter == 1) {
		lfo2_led_update();
		blink_counter--;
	}
}
void lfo2_process() {
	lfo2_but_process();

	if(LFO2SYNC && !lfo2_prev_sync) {
		if(lfo2_sync_cnt == 0) {
			lfo2_reset();
		}

		if(lfo2_mode == 1 || lfo2_mode == 2) {
			lfo2_sync_cnt++;
			if(lfo2_sync_cnt > 1 && lfo2_mode == 1) {lfo2_sync_cnt = 0;}
			if(lfo2_sync_cnt > 3 && lfo2_mode == 2) {lfo2_sync_cnt = 0;}
		} else {lfo2_sync_cnt = 0;}
	}
	lfo2_prev_sync = LFO2SYNC;

	uint16_t lfo_current_wf = adc_get_value(ADC_LFO2_WF);
	uint16_t lfoWF = (lfo2_prev_wf * 15 + lfo_current_wf)>>4;
	lfo2_prev_wf = lfoWF;

	uint16_t lfo_current_freq = adc_get_value(ADC_LFO2_SPD);
	uint16_t lfoFreq = (lfo2_prev_freq * 15 + lfo_current_freq)>>4;
	lfo2_prev_freq = lfoFreq;

	lfo2_phase += lfo2_inc(lfoFreq);
	uint16_t lfoPhaseQ = 2047 & (lfo2_phase >> (24-11));

	GPIOA->BSRR |= ((lfoPhaseQ >>10) == 0) ? GPIO_BSRR_BS_9 : GPIO_BSRR_BR_9;

	if((lfoPhaseQ >> 10) == 0 && lfo2_prev_sq == 1) {
		lfo2_rungler_process();
	}
	lfo2_prev_sq = lfoPhaseQ >> 10;

	if(lfoWF == 0) {
		LFO2OUT = (lfoPhaseQ >> 1); // Rising saw
	} else if (lfoWF < 512) {
		lfoWF = lfoWF << 2; // From 0-512 to 0-2048

		// For inverse phase
//		if(lfoPhaseQ < lfoWF) {
//			LFO2OUT = 1023 - lfoPhaseQ * 1023 / lfoWF;
//		} else {
//			LFO2OUT = (lfoPhaseQ - lfoWF) * 1023 / (2047-lfoWF);
//		}

		// For normal phase
		if(lfoPhaseQ < 2048-lfoWF) {
			LFO2OUT = (lfoPhaseQ) * 1023 / (2047-lfoWF);
		} else {
			LFO2OUT = 1023 - (lfoPhaseQ-(2048-lfoWF)) * 1023 / lfoWF;
		}
	} else {
		lfoWF = 2047 - ((lfoWF - 512) << 2);
		if(lfoPhaseQ == 0) {
			LFO2OUT = 1023;
		}
		else if(lfoWF < 128) {
			LFO2OUT = lfoPhaseQ < 128 ? 1023 - (lfoPhaseQ) * 8 : 0;
		}
		else if(lfoPhaseQ < lfoWF) {
			LFO2OUT = 1023 - (lfoPhaseQ) * 1023 / lfoWF; // Falling saw
		}
	}
}

void lfo2_update() {
	if(lfo2_save_eeprom) {
		uint8_t dat = lfo2_mode | (lfo2_rungler_length << 2);
		uint8_t buf[5] = {dat, 0, 0, 0, 0};
		memcpy(&buf[1], &lfo2_rungler_init, 4);

		eeprom_wait_available(100);
		eeprom_write_buffer(EEPROM_LFO2_DAT, buf, 5);
		//_delayMs(6);
		lfo2_save_eeprom = 0;
	}
}
