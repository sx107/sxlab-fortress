/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "usb_device.h"
#include "btldr_config.h"
#include "ihex_parser.h"
#include "drv/led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
extern PCD_HandleTypeDef hpcd_USB_FS;






/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


bool is_button_down(void)
{
    return (!LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_4));
}

bool is_appcode_exist(void)
{
  uint32_t *mem = (uint32_t*)APP_ADDR;

  if ((mem[0] == 0x00000000 || mem[0] == 0xFFFFFFFF) && \
      (mem[1] == 0x00000000 || mem[1] == 0xFFFFFFFF) && \
      (mem[2] == 0x00000000 || mem[2] == 0xFFFFFFFF) && \
      (mem[3] == 0x00000000 || mem[3] == 0xFFFFFFFF))
  {
    return false;
  }
  else
  {
    return true;
  }
}

void SystemReset(void){
    LL_mDelay(500);

    HAL_PCD_Stop(&hpcd_USB_FS);

    /* Pull USB D+ PIN to GND so USB Host detects device disconnect */
    LL_GPIO_SetPinMode(GPIOA,LL_GPIO_PIN_12, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_12, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinOutputType(GPIOA,LL_GPIO_PIN_12, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_12);

    LL_mDelay(1000);

    NVIC_SystemReset();
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

	static uint32_t jump_addr;
	//

	MX_GPIO_Init();
	for(uint32_t i = 0; i < 10000; i++) {
		__asm("nop");
	}

  if(is_button_down()) {
	  SystemClock_Config();
	  HAL_Init();
	  MX_USB_DEVICE_Init();
	  LL_mDelay(10);
	  led_init();


		uint32_t t = 0;

	  while(1) {
		for(uint8_t i = 0; i < LED_NUMBER; i++) {
			uint16_t br = 15 - ((((t >> 1) + i * 44) & 0x3FF) >> 7);
			if (br > 10) {br -= 10;}
			else {br = 0;}
			br <<= 1;
			led_set(i, 0xFFFFFF, br);
		}

		  led_update();

		  t++;

	      if(ihex_is_eof()) {
	         SystemReset();
	      }
	  }
  } else {

		jump_addr = *((__IO uint32_t*)(APP_ADDR+4u));
		HAL_DeInit();
		// Disable all interrupts
		NVIC->ICER[0] = 0xFFFFFFFF;
		NVIC->ICER[1] = 0xFFFFFFFF;
		NVIC->ICER[2] = 0xFFFFFFFF;
		NVIC->ICER[3] = 0xFFFFFFFF;
		NVIC->ICER[4] = 0xFFFFFFFF;
		NVIC->ICER[5] = 0xFFFFFFFF;
		NVIC->ICER[6] = 0xFFFFFFFF;
		NVIC->ICER[7] = 0xFFFFFFFF;

		NVIC->ICPR[0] = 0xFFFFFFFF;
		NVIC->ICPR[1] = 0xFFFFFFFF;
		NVIC->ICPR[2] = 0xFFFFFFFF;
		NVIC->ICPR[3] = 0xFFFFFFFF;
		NVIC->ICPR[4] = 0xFFFFFFFF;
		NVIC->ICPR[5] = 0xFFFFFFFF;
		NVIC->ICPR[6] = 0xFFFFFFFF;
		NVIC->ICPR[7] = 0xFFFFFFFF;


		/* deactivate SysTick */
		SysTick->CTRL = 0;

		/* Change the main stack pointer. */
		SCB->VTOR = APP_ADDR;
		__set_MSP((*(__IO uint32_t*)APP_ADDR));
		__DSB();
		((void (*) (void)) (jump_addr)) ();

  }

  while (1)
  {

  }

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_2)
  {
  }
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE_DIV_1, LL_RCC_PLL_MUL_9);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_SetSystemCoreClock(72000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL_DIV_1_5);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

  /**/
  GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
