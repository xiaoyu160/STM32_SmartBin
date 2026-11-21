#ifndef __BEEP_H
#define __BEEP_H

#include "stm32f10x.h"

/* 板载蜂鸣器驱动：引脚与极性见 BoardConfig.h */
void Beep_Init(void);       /* 推挽输出，默认不响 */
void Beep_On(void);
void Beep_Off(void);

#endif
