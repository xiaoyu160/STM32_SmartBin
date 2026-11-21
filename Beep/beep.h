#ifndef __BEEP_H
#define __BEEP_H

#include "stm32f10x.h"
#define BEEP_PORT GPIOA
#define BEEP_PIN  GPIO_Pin_4

void Beep_Init(void);
void Beep_Demo(void);
void Beep_On(void);
void Beep_Off(void);
void Beep_Toggle(void);

#endif
