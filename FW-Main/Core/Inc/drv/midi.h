/*
 * midi.h
 *
 *  Created on: 27 февр. 2024 г.
 *      Author: Sx107
 */

#ifndef INC_DRV_MIDI_H_
#define INC_DRV_MIDI_H_

#include <main.h>

extern volatile uint8_t own_trs_channel;
extern volatile uint8_t own_usb_channel;
extern volatile uint8_t midi_trs_learn;
extern volatile uint8_t midi_usb_learn;

typedef enum {
	MIDI_MODE_RESET, // Waiting for the 1st status byte, ignoring all data bytes
	MIDI_MODE_SINGLE_WAIT, // Waiting for a single byte
	MIDI_MODE_1BYTE_WAIT, // Waiting for the 1st data byte for a 2-byte message
	MIDI_MODE_2BYTE_WAIT, // Waiting for the second data byte in a 2-byte message
	MIDI_MODE_SYSEX // Receiving a SYSEX, ignoring it, waiting for SysEx end (0xF0)
} midi_mode_t;

typedef enum {
	MIDI_USB,
	MIDI_UART
} midi_source_t;

#define MIDI_CHAN(X) (X & 0x0F)	// Returns the channel from the status byte
#define MIDI_CMD(X) (X & 0xF0)	// Returns the command from the status byte

void midi_init();
void midi_parse_usb_message(uint32_t data);

uint8_t midi_get_note();
uint16_t midi_get_pb();
uint8_t midi_get_note_velocity();
uint8_t midi_get_cc(uint8_t cc_number);

// For CCs See adc_get_value()

void midi_usb_connect();
void midi_usb_disconnect();

void midi_uart_update();
void midi_usb_update();

#endif /* INC_DRV_MIDI_H_ */
