#include <main.h>
#include <stdint.h>

#include <drv/rcc.h>
#include <drv/adc.h>
#include <drv/i2c.h>
#include <drv/led.h>
#include <drv/proc_adc.h>
#include <drv/buttons.h>

#include <drv/eeprom.h>
#include <drv/update.h>
#include <drv/midi.h>

#include <osc_proc_core.h>
#include <lfo_timer.h>
#include <lfo1.h>
#include <lfo2.h>

#include "usb_device.h"

/*
CC70 MODE
CC71 WAVEFORM
CC72 TIMBRE

CC74 PROC KNOB
CC75 RESONANCE

CC76 LFO1 SPD
CC77 LFO1 WF
CC78 LFO2 SPD
CC79 LFO2 WF

CC82 SUB1 WAVEFORM
CC83 SUB1 FREQ
CC84 PORTAMENTO
*/

void settings_mode(); // settings_mode.c

int main(void)
{
	SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
	memcpy((uint32_t*)0x10000000, (uint32_t*)(FLASH_BASE + 0x5000), 404);
	SCB->VTOR = 0x10000000;

	//SCB->VTOR = FLASH_BASE | (0x00005000U);
	__set_MSP((*(__IO uint32_t*)(FLASH_BASE | (0x00005000U))));

	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	// Initialize clock
	RCC->AHBENR |= RCC_AHBENR_GPIOFEN;
	rcc_init();
	_delayMs(10);

	#if USE_DWT
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
		DWT->CYCCNT = 0;
	#endif

	RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

	// Init main drivers
	buttons_init();

	adc_init();
	i2c1_init();
	led_init();
	midi_init();

	if (LFO1BUT) {
		_delayMs(50); // Do not delete. We must wait while ADC does some stuff
		if(LFO1BUT) {
			MX_USB_DEVICE_Init();
			settings_mode();
		}
	}
	// Rest of the initialization
	_delayMs(50);
	MX_USB_DEVICE_Init();
	_delayMs(10);
	proc_adc_init();
	_delayMs(10);
	lfo1_init();
	lfo2_init();
	_delayMs(10);
	lfo_timer_init();
	osc_proc_init();
	update_init();



	for(;;) {
		//midi_usb_update();
		midi_uart_update();
		__WFI();
	}

}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
