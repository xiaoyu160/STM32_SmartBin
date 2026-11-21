#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "stm32f10x.h"
extern volatile uint32_t sys_ms;
extern volatile uint16_t tick_cnt;
extern volatile uint8_t flag_10ms;
extern volatile uint8_t flag_100ms;
extern volatile uint8_t flag_1s;
void SysTick_Init(void);

#endif
