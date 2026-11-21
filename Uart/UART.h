#ifndef __UART_H
#define __UART_H

#include "stm32f10x.h"
void USART3_Init(uint32_t baud);
void USART3_SendString(char *s);
void Parse_Cmd(char *cmd);
void Uart_Demo(void);

#endif
