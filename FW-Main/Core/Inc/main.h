/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f3xx.h"
#include <stdint.h>


/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

#include "stm32f3xx_ll_rcc.h"
#include "stm32f3xx_ll_bus.h"
#include "stm32f3xx_ll_system.h"
#include "stm32f3xx_ll_exti.h"
#include "stm32f3xx_ll_cortex.h"
#include "stm32f3xx_ll_utils.h"
#include "stm32f3xx_ll_pwr.h"
#include "stm32f3xx_ll_dma.h"
#include "stm32f3xx_ll_gpio.h"

#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007)
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006)
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005)
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004)
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003)
#endif


#define WAITNBIT(REG, BIT) while(((REG) & (BIT)) == (BIT)) {};
#define WAITBIT(REG, BIT) while(((REG) & (BIT)) != (BIT)) {};
#define WAITBITS(REG, BIT) while((((REG) & (BIT)) & (BIT)) == 0) {};

#define WAITNBITTMO(REG, BIT, TMO) for(uint32_t __tmo = 0; __tmo < TMO; __tmo++) {if(((REG) & (BIT)) != (BIT)) {break;}}
#define WAITBITTMO(REG, BIT, TMO) for(uint32_t __tmo = 0; __tmo < TMO; __tmo++) {if(((REG) & (BIT)) == (BIT)) {break;}}

#define WAITVAL(REG, BIT, VAL) while(((REG) & (BIT)) != (VAL)) {};
#define RETERR(F, E) if(F != 0) {return E;}

#define SIMPLETMO(XTMO) for(uint32_t __tmo = 0; __tmo < XTMO*72/4; __tmo++) {__asm("NOP");}

#define USE_DWT 0


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
