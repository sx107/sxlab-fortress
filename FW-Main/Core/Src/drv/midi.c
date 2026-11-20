/*
 * midi.c
 *
 *  Created on: 27 февр. 2024 г.
 *      Author: Sx107
 */
#include <drv/eeprom.h>
#include <drv/midi.h>
#include <lfo2.h>

volatile uint8_t usb_midi_cmd = 0;
volatile uint8_t usb_midi_fb = 0;
volatile midi_mode_t usb_midi_state;

volatile uint8_t uart_midi_cmd = 0;
volatile uint8_t uart_midi_fb = 0;
volatile midi_mode_t uart_midi_state;

volatile uint32_t midi_sync_pos = 0;
volatile uint16_t midi_pitchbend = 8192;

#define MIDI_IS_STAT(X) ((X & 0x80) == 0x80)
#define MIDI_IS_DATA(X) ((X & 0x80) == 0)

#define MIDI_GATE_HIGH	GPIOC->BSRR |= GPIO_BSRR_BS_12
#define MIDI_GATE_LOW	GPIOC->BSRR |= GPIO_BSRR_BR_12

#define MIDI_SYNC_HIGH	GPIOD->BSRR |= GPIO_BSRR_BS_2
#define MIDI_SYNC_LOW	GPIOD->BSRR |= GPIO_BSRR_BR_2

volatile uint8_t _midi_velos[128];
volatile uint8_t _midi_ccs[128];

volatile uint8_t _midi_usb_stack_backwards[128];
volatile uint8_t _midi_usb_stack_forwards[128];
volatile uint8_t _midi_usb_last_note = 0xFF;

volatile uint8_t _midi_uart_stack_backwards[128];
volatile uint8_t _midi_uart_stack_forwards[128];
volatile uint8_t _midi_uart_last_note = 0xFF;

volatile uint8_t midi_note = 0xFF;

volatile uint8_t _midi_uart_buffer[1024];
volatile uint_fast16_t _midi_uart_buffer_pos_wr = 0;
volatile uint_fast16_t _midi_uart_buffer_pos_rd = 0;
volatile uint_fast16_t _midi_uart_buffer_len = 0;

// volatile uint8_t _midi_usb_buffer[1024];
// volatile uint_fast16_t _midi_usb_buffer_pos_wr = 0;
// volatile uint_fast16_t _midi_usb_buffer_pos_rd = 0;
// volatile uint_fast16_t _midi_usb_buffer_len = 0;

volatile uint8_t own_trs_channel = 0xFF;
volatile uint8_t own_usb_channel = 0xFF;
volatile uint8_t midi_trs_learn = 0;
volatile uint8_t midi_usb_learn = 0;

void midi_set_to_last(midi_source_t priority) {
	if(priority == MIDI_USB) {
		// USB has priority.
		if(_midi_usb_last_note != 0xFF) {midi_note = _midi_usb_last_note;}
		else {midi_note = _midi_uart_last_note;}
	} else {
		// UART has priority.
		if(_midi_uart_last_note != 0xFF) {midi_note = _midi_uart_last_note;}
		else {midi_note = _midi_usb_last_note;}
	}

	// Check if any of them has a note. If yes, output the gate.
	if(_midi_usb_last_note != 0xFF || _midi_uart_last_note != 0xFF) {
		MIDI_GATE_HIGH;
	} else {
		MIDI_GATE_LOW;
	}
}

void _midi_usb_push_note(uint8_t note) {
	if(_midi_usb_last_note == note || _midi_usb_stack_backwards[note] != 0xFF || _midi_usb_stack_forwards[note] != 0xFF) {return;}
	if(_midi_usb_last_note != 0xFF) {_midi_usb_stack_forwards[_midi_usb_last_note] = note; _midi_usb_stack_backwards[note] = _midi_usb_last_note;}
	else {_midi_usb_stack_backwards[note] = 0xFF;}
	_midi_usb_stack_forwards[note] = 0xFF;
	_midi_usb_last_note = note;
}
void _midi_usb_pop_note(uint8_t note) {
	if (note ==  _midi_usb_last_note) {
		_midi_usb_last_note = _midi_usb_stack_backwards[note];
		_midi_usb_stack_backwards[note] = 0xFF;
		_midi_usb_stack_forwards[note] = 0xFF;
		if(_midi_usb_last_note != 0xFF) {_midi_usb_stack_forwards[_midi_usb_last_note] = 0xFF;}
		return;
	}

	if(_midi_usb_stack_backwards[note] != 0xFF) {
		_midi_usb_stack_forwards[_midi_usb_stack_backwards[note]] = _midi_usb_stack_forwards[note];
	}
	if(_midi_usb_stack_forwards[note] != 0xFF) {
		_midi_usb_stack_backwards[_midi_usb_stack_forwards[note]] = _midi_usb_stack_backwards[note];
	}

	_midi_usb_stack_backwards[note] = 0xFF;
	_midi_usb_stack_forwards[note] = 0xFF;
}
void midi_usb_stack_clear() {
	for(uint8_t i = 0; i < 128; i++) {
		_midi_usb_stack_forwards[i] = 0xFF;
		_midi_usb_stack_backwards[i] = 0xFF;
	}
	_midi_usb_last_note = 0xFF;
	midi_set_to_last(MIDI_USB);
}

void _midi_uart_push_note(uint8_t note) {
	if(_midi_uart_last_note == note || _midi_uart_stack_backwards[note] != 0xFF || _midi_uart_stack_forwards[note] != 0xFF) {return;}
	if(_midi_uart_last_note != 0xFF) {_midi_uart_stack_forwards[_midi_uart_last_note] = note; _midi_uart_stack_backwards[note] = _midi_uart_last_note;}
	else {_midi_uart_stack_backwards[note] = 0xFF;}
	_midi_uart_stack_forwards[note] = 0xFF;
	_midi_uart_last_note = note;
}
void _midi_uart_pop_note(uint8_t note) {
	if (note ==  _midi_uart_last_note) {
		_midi_uart_last_note = _midi_uart_stack_backwards[note];
		_midi_uart_stack_backwards[note] = 0xFF;
		_midi_uart_stack_forwards[note] = 0xFF;
		if(_midi_uart_last_note != 0xFF) {_midi_uart_stack_forwards[_midi_uart_last_note] = 0xFF;}
		return;
	}

	if(_midi_uart_stack_backwards[note] != 0xFF) {
		_midi_uart_stack_forwards[_midi_uart_stack_backwards[note]] = _midi_uart_stack_forwards[note];
	}
	if(_midi_uart_stack_forwards[note] != 0xFF) {
		_midi_uart_stack_backwards[_midi_uart_stack_forwards[note]] = _midi_uart_stack_backwards[note];
	}

	_midi_uart_stack_backwards[note] = 0xFF;
	_midi_uart_stack_forwards[note] = 0xFF;
}
void midi_uart_stack_clear() {
	for(uint8_t i = 0; i < 128; i++) {
		_midi_uart_stack_forwards[i] = 0xFF;
		_midi_uart_stack_backwards[i] = 0xFF;
	}
	_midi_uart_last_note = 0xFF;
	midi_set_to_last(MIDI_UART);
}



uint8_t midi_get_note() {
	if (midi_note > 108) {return 0xFF;}
	return midi_note;
}

uint8_t midi_get_note_velocity() {
	if (midi_note > 108) {return 0xFF;}
	return _midi_velos[midi_note];
}

uint16_t midi_get_pb() {
	return midi_pitchbend;
}

uint8_t midi_get_cc(uint8_t cc_number) {
	if (cc_number == 0xFF) {return 0;}
	return _midi_ccs[cc_number & 0x7F];
}

void midi_clean_ccs() {
	for(uint8_t i = 0; i < 128; i++) {
		_midi_ccs[i] = 0;
	}
}

void midi_init() {
	// Read own channel
	own_trs_channel = eeprom_read8(EEPROM_MIDI_TRS_CHAN);
	if (own_trs_channel > 15) {own_trs_channel = 0xFF;}
	own_usb_channel = eeprom_read8(EEPROM_MIDI_USB_CHAN);
	if (own_usb_channel > 15) {own_usb_channel = 0xFF;}


	// Stack clear
	midi_usb_stack_clear();
	midi_uart_stack_clear();
	midi_clean_ccs();

	// Mod output
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER8_Msk, (0b10) << GPIO_MODER_MODER8_Pos);
	GPIOA->AFR[1] |= (6 << GPIO_AFRH_AFRH0_Pos);

	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	MODIFY_REG(TIM1->CR1, TIM_CR1_CMS_Msk | TIM_CR1_CKD_Msk | TIM_CR1_DIR_Msk, TIM_CR1_ARPE);

	TIM1->PSC = 0;
	TIM1->ARR = 127;
	TIM1->CCR1 = 0;
	TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
	TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
	TIM1->CR1 |= TIM_CR1_CEN;
	TIM1->EGR |= TIM_EGR_UG;

	// Gate output
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER12_Msk, (0b01) << GPIO_MODER_MODER12_Pos);
	MIDI_GATE_LOW;

	// Sync output
	RCC->AHBENR |= RCC_AHBENR_GPIODEN;
	MODIFY_REG(GPIOD->MODER, GPIO_MODER_MODER2_Msk, (0b01) << GPIO_MODER_MODER2_Pos);
	MIDI_SYNC_LOW;

	// UART Input
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER7_Msk, (0b10) << GPIO_MODER_MODER7_Pos);
	GPIOB->AFR[0] |= (7 << GPIO_AFRL_AFRL7_Pos);

	// UART Interrupt
	NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
	NVIC_EnableIRQ(USART1_IRQn);

	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	USART1->BRR = 2304; //72000000 / 31250
	MODIFY_REG(USART1->CR1, USART_CR1_M_Msk | USART_CR1_PCE_Msk | USART_CR1_TE_Msk, USART_CR1_RE);
	MODIFY_REG(USART1->CR2, USART_CR2_STOP_Msk, 0 << USART_CR2_STOP_Pos);
	USART1->CR1 |= USART_CR1_UE | USART_CR1_RXNEIE;
}

volatile uint8_t is_sync_running = 0;

void midi_sync_tick() {
	if (is_sync_running) {
		if(midi_sync_pos < 12) {MIDI_SYNC_HIGH;}
		else {MIDI_SYNC_LOW;}

		midi_sync_pos += 1;
		if(midi_sync_pos >= 24) {midi_sync_pos = 0;}
	}
}
void midi_sync_start() {
	midi_sync_pos = 0;
	is_sync_running = 1;
}

void midi_sync_stop() {
	is_sync_running = 0;
	lfo2_midi_reset();
}

void midi_sync_continue() {
	is_sync_running = 1;
}

void midi_sync_setpos(uint16_t pos) {
	midi_sync_pos = (pos * 6) % 24;
}

void handle_midi_message(uint8_t cmd, uint8_t fb, uint8_t sb, midi_source_t source) {
	if(cmd == 0xF2) {
		uint16_t song_pos = (fb & 0x7F) | ((sb & 0x7F) << 7);
		midi_sync_setpos(song_pos);
		return;
	}

	if (midi_usb_learn && source == MIDI_USB && MIDI_CMD(cmd) != 0xF0) {
		own_usb_channel = MIDI_CHAN(cmd);
		midi_usb_learn = 0;
	}
	if (midi_trs_learn && source == MIDI_UART && MIDI_CMD(cmd) != 0xF0) {
		own_trs_channel = MIDI_CHAN(cmd);
		midi_trs_learn = 0;
	}

	if (source == MIDI_USB && own_usb_channel != 0xFF && MIDI_CHAN(cmd) != own_usb_channel) {return;}
	if (source == MIDI_UART && own_trs_channel != 0xFF && MIDI_CHAN(cmd) != own_trs_channel) {return;}

	switch(MIDI_CMD(cmd)) {
		case 0xB0:
			_midi_ccs[fb] = sb;
			if (fb == 1) {TIM1->CCR1 = sb & 0x7F;}	// CC1: Modulation
			else if (fb == 123) {
				// CC123: All notes off
				//midi_clean_ccs();
				midi_usb_stack_clear();
				midi_uart_stack_clear();
				_midi_uart_buffer_len = 0;
				_midi_uart_buffer_pos_wr = 0;
				_midi_uart_buffer_pos_rd = 0;
				//NVIC_SystemReset();
			}
			break;
		case 0xE0:
			// Set pitchbend
			midi_pitchbend = (uint16_t)(fb & 0x7F) + ((uint16_t)(sb & 0x7F) << 7);
			break;
		case 0x90:
			// Add note to list.
			_midi_velos[fb & 0x7F] = sb;
			if(source == MIDI_USB) {_midi_usb_push_note(fb);}
			else {_midi_uart_push_note(fb);}
			midi_set_to_last(source);
			break;
		case 0x80:
			// Delete note from list.

			if(source == MIDI_USB) {_midi_usb_pop_note(fb);}
			else {_midi_uart_pop_note(fb);}
			midi_set_to_last(source);
			break;
	}
}

void parse_midi_byte(uint8_t rb, midi_source_t source) {
	volatile uint8_t* _m_cmd;
	volatile uint8_t* _m_fb;
	volatile midi_mode_t* _m_state;

	if(source == MIDI_USB) {
		_m_cmd = &usb_midi_cmd;
		_m_fb = &usb_midi_fb;
		_m_state = &usb_midi_state;
	} else {
		_m_cmd = &uart_midi_cmd;
		_m_fb = &uart_midi_fb;
		_m_state = &uart_midi_state;
	}


	// Single-byte system common messages
	if(rb == 0xF8 || rb == 0xFA || rb == 0xFB || rb == 0xFC || rb == 0xFE || rb == 0xFF) {
		// 0xF2 handled in handle_midi_message - not single-byte
		if(rb == 0xF8) {midi_sync_tick();}
		else if(rb == 0xFA) {midi_sync_start();}
		else if(rb == 0xFB) {midi_sync_continue();}
		else if(rb == 0xFC) {midi_sync_stop();}
		return;
	}

	if(rb == 0xF0) {*_m_state = MIDI_MODE_SYSEX; return;}	// SysEx start
	if(rb == 0xF7) {*_m_state = MIDI_MODE_RESET; return;}	// SysEx end
	if(*_m_state == MIDI_MODE_SYSEX) {return;}				// Ignore if we are in a SysEx mode

	if((*_m_state == MIDI_MODE_RESET) && MIDI_IS_DATA(rb)) {
		// Simply wait. We don't know the running status at this point anyways
		return;
	}



	if(MIDI_IS_STAT(rb)) {
		*_m_cmd = rb;
		uint8_t cmd = MIDI_CMD(rb);
		if (rb == 0xF2) {*_m_state = MIDI_MODE_1BYTE_WAIT;} // 0xF2 song position 2-byte
		else if(cmd == 0xF0) {*_m_state = MIDI_MODE_RESET;} // Unknown system common message, ignore!
		else if (cmd == 0xC0 || cmd == 0xD0) {*_m_state = MIDI_MODE_SINGLE_WAIT;} // Wait for a single byte
		else {*_m_state = MIDI_MODE_1BYTE_WAIT;}
	} else {
		if(*_m_state == MIDI_MODE_1BYTE_WAIT) {
			*_m_fb = rb;
			*_m_state = MIDI_MODE_2BYTE_WAIT;
		} else if (*_m_state == MIDI_MODE_2BYTE_WAIT) {
			*_m_state = MIDI_MODE_1BYTE_WAIT;
			if ((MIDI_CMD(*_m_cmd) == 0x90) && (rb == 0)) { // Implicit note off
				handle_midi_message(0x80 | MIDI_CHAN(*_m_cmd), *_m_fb, 0, source);
			} else {
				handle_midi_message(*_m_cmd, *_m_fb, rb, source);
			}
		} else if (*_m_state == MIDI_MODE_SINGLE_WAIT) {
			handle_midi_message(*_m_cmd, rb, 0, source);
		}
	}
}


void midi_parse_usb_message(uint32_t data) {
	uint8_t usb_type = data & 0x0F;
	uint8_t bytes[3];
	bytes[0] = (data >> 8) & 0xFF;
	bytes[1] = (data >> 16) & 0xFF;
	bytes[2] = (data >> 24) & 0xFF;

	uint8_t byte_len;


	if (usb_type == 0x0F || usb_type == 0x05) {byte_len = 1;}
	else if(usb_type == 0x02 || usb_type == 0x0C || usb_type == 0x0D || usb_type == 0x06) {byte_len = 2;}
	else {byte_len = 3;}


	for(uint8_t i = 0; i < byte_len; i++) {
		parse_midi_byte(bytes[i], MIDI_USB);

		//_midi_usb_buffer[_midi_usb_buffer_pos_wr] = bytes[i];
		//_midi_usb_buffer_pos_wr = (_midi_usb_buffer_pos_wr + 1) & 0x1FF;
		//_midi_usb_buffer_len += 1;
	}
}

void midi_usb_connect() {

}
void midi_usb_disconnect() {
	midi_usb_stack_clear();
}

void midi_uart_update() {
	while (_midi_uart_buffer_pos_rd != _midi_uart_buffer_pos_wr) {
		parse_midi_byte(_midi_uart_buffer[_midi_uart_buffer_pos_rd], MIDI_UART);
		_midi_uart_buffer_pos_rd = (_midi_uart_buffer_pos_rd + 1) & 0x1FF;
		_midi_uart_buffer_len -= 1;
		if (_midi_uart_buffer_len >= 0xFFFFFF00) {
			_midi_uart_buffer_len = 0;
			_midi_uart_buffer_pos_rd = 0;
			_midi_uart_buffer_pos_wr = 0;
		}
	}
}

/*
void midi_usb_update() {
	while (_midi_usb_buffer_pos_rd != _midi_usb_buffer_pos_wr) {
		parse_midi_byte(_midi_usb_buffer[_midi_usb_buffer_pos_rd], MIDI_USB);
		_midi_usb_buffer_pos_rd = (_midi_usb_buffer_pos_rd + 1) & 0x1FF;
		_midi_usb_buffer_len -= 1;
		if (_midi_usb_buffer_len >= 0xFFFFFF00) {
			_midi_usb_buffer_len = 0;
			_midi_usb_buffer_pos_rd = 0;
			_midi_usb_buffer_pos_wr = 0;
		}
	}
}
*/


void USART1_IRQHandler() {
	uint8_t rb = USART1->RDR;
	//parse_midi_byte(rb, MIDI_UART);

	_midi_uart_buffer[_midi_uart_buffer_pos_wr] = rb;
	_midi_uart_buffer_pos_wr = (_midi_uart_buffer_pos_wr + 1) & 0x1FF;
	_midi_uart_buffer_len += 1;

	if(USART1->ISR & USART_ISR_ORE) {
		uart_midi_cmd = 0;
		uart_midi_fb = 0;
		uart_midi_state = MIDI_MODE_RESET;
	}
	USART1->ICR |= USART_ICR_ORECF;
}

