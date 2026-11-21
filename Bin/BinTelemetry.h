#ifndef __BIN_TELEMETRY_H
#define __BIN_TELEMETRY_H

#include "stm32f10x.h"

/* 串口上报（BIN_USE_UART = 0 时全部编译成空函数，不占代码空间）
 *
 * 输出格式（一行一条，逗号分隔，方便上位机/串口助手直接看）：
 *   开机   BIN,BOOT,SmartBin v1.0
 *   开盖   EVT,OPEN,<IR|KEY>,cnt=<次数>
 *   关盖   EVT,CLOSE
 *   满溢   EVT,FULL / EVT,CLEAR
 *   投放   EVT,THROW
 *   心跳   BIN,lid=0,full=0,open=1,throw=1,up=12s */

void BinTelemetry_Boot(void);
void BinTelemetry_Event(const char *name);
void BinTelemetry_LidOpen(const char *why, uint32_t open_cnt);
void BinTelemetry_Heartbeat(uint8_t lid_open, uint8_t full,
                            uint32_t open_cnt, uint32_t throw_cnt);

#endif
