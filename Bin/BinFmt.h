#ifndef __BIN_FMT_H
#define __BIN_FMT_H

#include "stm32f10x.h"

/* 极简整数格式化：把数字直接写进调用者的缓冲区，不引入 printf
 *
 * 用法（链式，返回新的写指针）：
 *     char buf[32]; char *p = buf;
 *     p = BinFmt_Str(p, "cnt=");
 *     p = BinFmt_U32(p, cnt, 3, '0');
 *     *p = '\0';
 *
 * 调用者要保证缓冲区够大；这里不做边界检查（嵌入式里由调用方算好）。 */

char *BinFmt_Str(char *p, const char *s);
char *BinFmt_U32(char *p, uint32_t v, uint8_t width, char pad);
char *BinFmt_Sec(char *p, uint32_t ms);     /* 毫秒 -> "12.3"（一位小数） */

#endif
