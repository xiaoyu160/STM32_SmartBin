#ifndef __OLED_H
#define __OLED_H

#include "stm32f10x.h"

/* 0.96 寸 128x64 OLED（SSD1306），软件 I2C
 * 引脚见 BoardConfig.h（PB8 = SCL，PB9 = SDA）
 * 坐标约定：Line 从上到下 1~4，Column 从左到右 1~16 */

void OLED_Init(void);                                       /* 含上电延时，最后自动清屏 */
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String);
void OLED_ShowChinese(uint8_t Line, uint8_t Column, uint8_t Index);

#endif
