/*
 * settings_mode.c
 *
 *  Created on: 14 июл. 2025 г.
 *      Author: Sx107
 */

#include <main.h>
#include <drv/rcc.h>
#include <drv/adc.h>
#include <drv/led.h>
#include <drv/midi.h>
#include <drv/buttons.h>
#include <drv/eeprom.h>

void settings_mode() {
	uint8_t midi_chan_sel = 0; // 0 = USB, 1 = TRS
	uint32_t midi_sel_color = 0xFF0000;

	uint8_t prev_trs_channel = own_trs_channel;
	uint8_t prev_usb_channel = own_usb_channel;

	uint8_t prev_but_proc = 0;
	uint16_t but_proc_timer = 0;
	uint8_t but_proc_ignore_next = 0;

	uint16_t learn_anim = 0;

	if (adc_raw_value(ADC_PROC_KNOB) < 512) {midi_chan_sel = 0; midi_sel_color = 0xFF0000;}
	else {midi_chan_sel = 1; midi_sel_color = 0x0000FF;}

	while(1) {
		if (adc_raw_value(ADC_PROC_KNOB) < 512 - 128) {midi_chan_sel = 0; midi_sel_color = 0xFF0000;}
		else if (adc_raw_value(ADC_PROC_KNOB) > 512 + 128) {midi_chan_sel = 1; midi_sel_color = 0x0000FF;}
		led_set(LED_PROC_KNOB, midi_sel_color, LED_BRIGHTNESS);

		if(BUT_PROC) {
			if (but_proc_timer < 200) {
				but_proc_timer += 1;
			}
			if (but_proc_timer == 200) {
				if (midi_chan_sel == 0) {prev_usb_channel = 0xFF; own_usb_channel = 0xFF; midi_usb_learn = 1;}
				else {prev_trs_channel = 0xFF; own_trs_channel = 0xFF; midi_trs_learn = 1;}
				but_proc_timer = 201;
				but_proc_ignore_next = 1;
			}
		} else {
			but_proc_timer = 0;
			if (prev_but_proc) {
				if (!but_proc_ignore_next) {
					if (midi_chan_sel == 0) {own_usb_channel++; midi_usb_learn = 0;}
					else {own_trs_channel++; midi_trs_learn = 0;}
					_delayMs(10);
				}
				but_proc_ignore_next = 0;
			}
		}
		prev_but_proc = BUT_PROC;

		if (own_usb_channel > 15) {own_usb_channel = 0xFF;}
		if (own_trs_channel > 15) {own_trs_channel = 0xFF;}

		if (midi_usb_learn) {
			uint8_t br = midi_chan_sel == 0 ? LED_BRIGHTNESS : 1;
			for (uint8_t i = 0; i < 4; i++) {
				led_set(LED_PROC_4 - i, ((learn_anim >> 6) & 0b11) == i ? 0xFFFFFF : 0x000000, br);
			}
			led_set(LED_PROC_0, 0x000000, br);
		} else {
			uint8_t br = midi_chan_sel == 0 ? LED_BRIGHTNESS : 1;

			if (own_usb_channel == 0xFF) {
				for (uint8_t i = 0; i < 4; i++) {
					led_set(LED_PROC_4 - i, 0x000000, br);
				}
				led_set(LED_PROC_0, 0xFF0000, br);
			} else {
				for (uint8_t i = 0; i < 4; i++) {
					led_set(LED_PROC_4 - i, ((own_usb_channel >> i) & 0b1) ? 0xFF0000 : 0x070707, br);
				}
				led_set(LED_PROC_0, 0x000000, br);
			}

			if (prev_usb_channel != own_usb_channel) {
				// If not currently in MIDI learn, and channel changed, remember it!
				led_set(LED_PROC_KNOB, 0xFFFFFF, LED_BRIGHTNESS);
				eeprom_wait_available(100);
				eeprom_write8(EEPROM_MIDI_USB_CHAN, own_usb_channel);
				prev_usb_channel = own_usb_channel;
				_delayMs(20);
				led_set(LED_PROC_KNOB, midi_sel_color, LED_BRIGHTNESS);
			}
		}

		if (midi_trs_learn) {
			uint8_t br = midi_chan_sel == 1 ? LED_BRIGHTNESS : 1;
			for (uint8_t i = 0; i < 4; i++) {
				led_set(LED_PROC_9 + i, ((learn_anim >> 6) & 0b11) == i ? 0xFFFFFF : 0x000000, br);
			}
			led_set(LED_PROC_5, 0x000000, br);
		} else {
			uint8_t br = midi_chan_sel == 1 ? LED_BRIGHTNESS : 1;

			if (own_trs_channel == 0xFF) {
				for (uint8_t i = 0; i < 4; i++) {
					led_set(LED_PROC_9 + i, 0x000000, br);
				}
				led_set(LED_PROC_5, 0x0000FF, br);
			} else {
				for (uint8_t i = 0; i < 4; i++) {
					led_set(LED_PROC_9 + i, ((own_trs_channel >> i) & 0b1) ? 0x0000FF : 0x070707, br);
				}
				led_set(LED_PROC_5, 0x000000, br);
			}

			if (prev_trs_channel != own_trs_channel) {
				// If not currently in MIDI learn, and channel changed, remember it!
				led_set(LED_PROC_KNOB, 0xFFFFFF, LED_BRIGHTNESS);
				eeprom_wait_available(100);
				eeprom_write8(EEPROM_MIDI_TRS_CHAN, own_trs_channel);
				prev_trs_channel = own_trs_channel;
				_delayMs(20);
				led_set(LED_PROC_KNOB, midi_sel_color, LED_BRIGHTNESS);
			}
		}

		learn_anim += 1;
		midi_uart_update();
		_delayMs(1);
	}

	NVIC_SystemReset(); // Just to be sure!
}
