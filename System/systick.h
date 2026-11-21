#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "stm32f10x.h"

/* 1ms 系统时基：全工程唯一的时间来源 */
extern volatile uint32_t sys_ms;

void SysTick_Init(void);
void SysTick_DelayMs(uint32_t ms);      /* 阻塞延时，仅限 SysTick_Init 之后 */

#endif
