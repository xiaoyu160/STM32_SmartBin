#include "stm32f10x.h"
#include "BinTelemetry.h"
#include "BinFmt.h"
#include "BinConfig.h"
#include "UART.h"
#include "systick.h"

#if BIN_USE_UART

void BinTelemetry_Boot(void)
{
    USART3_SendString("BIN,BOOT,SmartBin v1.0\r\n");
}

void BinTelemetry_Event(const char *name)
{
    char  buf[48];
    char *p = buf;

    p = BinFmt_Str(p, "EVT,");
    p = BinFmt_Str(p, name);
    p = BinFmt_Str(p, "\r\n");
    *p = '\0';
    USART3_SendString(buf);
}

void BinTelemetry_LidOpen(const char *why, uint32_t open_cnt)
{
    char  buf[32];
    char *p = buf;

    p = BinFmt_Str(p, "EVT,OPEN,");
    p = BinFmt_Str(p, why);
    p = BinFmt_Str(p, ",cnt=");
    p = BinFmt_U32(p, open_cnt, 1, '0');
    p = BinFmt_Str(p, "\r\n");
    *p = '\0';
    USART3_SendString(buf);
}

void BinTelemetry_Heartbeat(uint8_t lid_open, uint8_t full,
                            uint32_t open_cnt, uint32_t throw_cnt)
{
    char  buf[64];
    char *p = buf;

    p = BinFmt_Str(p, "BIN,lid=");
    p = BinFmt_U32(p, lid_open, 1, '0');
    p = BinFmt_Str(p, ",full=");
    p = BinFmt_U32(p, full, 1, '0');
    p = BinFmt_Str(p, ",open=");
    p = BinFmt_U32(p, open_cnt, 1, '0');
    p = BinFmt_Str(p, ",throw=");
    p = BinFmt_U32(p, throw_cnt, 1, '0');
    p = BinFmt_Str(p, ",up=");
    p = BinFmt_U32(p, sys_ms / 1000u, 1, '0');
    p = BinFmt_Str(p, "s\r\n");
    *p = '\0';
    USART3_SendString(buf);
}

#else   /* BIN_USE_UART = 0：保留接口，编译成空函数 */

void BinTelemetry_Boot(void) {}
void BinTelemetry_Event(const char *name) { (void)name; }
void BinTelemetry_LidOpen(const char *why, uint32_t open_cnt) { (void)why; (void)open_cnt; }
void BinTelemetry_Heartbeat(uint8_t lid_open, uint8_t full,
                            uint32_t open_cnt, uint32_t throw_cnt)
{
    (void)lid_open; (void)full; (void)open_cnt; (void)throw_cnt;
}

#endif
