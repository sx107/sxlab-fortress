/*
 * eeprom.h
 *
 *  Created on: Sep 18, 2023
 *      Author: Sx107
 */

#include <main.h>

#ifndef EEPROM_H_
#define EEPROM_H_

uint8_t eeprom_write8(uint8_t addr, uint8_t data);
uint8_t eeprom_read8(uint8_t addr);

uint8_t eeprom_write16(uint8_t addr, uint16_t data);
uint16_t eeprom_read16(uint8_t addr);

uint8_t eeprom_write32(uint8_t addr, uint32_t data);
uint32_t eeprom_read32(uint8_t addr);

uint8_t eeprom_write_buffer(uint8_t addr, const volatile uint8_t* buf, uint16_t len);
uint8_t eeprom_read_buffer(uint8_t addr, volatile uint8_t* buf, uint16_t len);

uint8_t eeprom_wait_available(uint16_t timeout);


// Common (not preset-wise)
#define EEPROM_LFO2_DAT 0
#define EEPROM_LFO2_RUNGLER 1
#define EEPROM_LFO1_DAT 5
#define EEPROM_LFO1_RUNGLER 6
#define EEPROM_OSC_PM 10
#define EEPROM_PROC_MODE 11
#define EEPROM_MIDI_TRS_CHAN 12
#define EEPROM_MIDI_USB_CHAN 13

#endif /* EEPROM_H_ */
