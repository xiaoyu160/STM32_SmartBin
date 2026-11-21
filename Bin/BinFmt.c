#include "stm32f10x.h"
#include "BinFmt.h"

#define FMT_MAX_DIGITS  12

char *BinFmt_Str(char *p, const char *s)
{
    while (*s) *p++ = *s++;
    return p;
}

/* 十进制无符号数，width 是最小宽度，不足时用 pad 补齐（'0' 或 ' '） */
char *BinFmt_U32(char *p, uint32_t v, uint8_t width, char pad)
{
    char    tmp[FMT_MAX_DIGITS];
    uint8_t n = 0;
    uint8_t i;

    if (v == 0)
    {
        tmp[n++] = '0';
    }
    while (v && n < FMT_MAX_DIGITS)
    {
        tmp[n++] = (char)('0' + (v % 10u));
        v /= 10u;
    }
    while (n < width && n < FMT_MAX_DIGITS)
    {
        tmp[n++] = pad;
    }
    for (i = n; i > 0; i--) *p++ = tmp[i - 1];
    return p;
}

/* 打印 1 位小数的秒数：ms -> "12.3" */
char *BinFmt_Sec(char *p, uint32_t ms)
{
    p = BinFmt_U32(p, ms / 1000u, 1, ' ');
    *p++ = '.';
    p = BinFmt_U32(p, (ms % 1000u) / 100u, 1, '0');
    return p;
}
