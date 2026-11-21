#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"

/* USART3 发送驱动：引脚见 BoardConfig.h（PB10 = TX，PB11 = RX） */
void USART3_Init(uint32_t baud);
void USART3_SendString(const char *s);

#endif
