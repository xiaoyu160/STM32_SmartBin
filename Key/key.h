#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"
#define KEY_PORT GPIOA
#define KEY1_PIN GPIO_Pin_2   /* K1: accelerate */
#define KEY2_PIN GPIO_Pin_4   /* K2: decelerate */
#define KEY3_PIN GPIO_Pin_6   /* K3: reverse */

void KEY_INIT(void);
int key_scan(void);
void key_demo(void);

#endif
