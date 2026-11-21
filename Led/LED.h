#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/* 板载 LED 驱动：引脚与极性见 BoardConfig.h */
void LED_Init(void);
void LED_Set(uint8_t on);       /* on = 1 点亮，0 熄灭 */

#endif
