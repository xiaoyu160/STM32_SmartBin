#ifndef __BIN_APP_H
#define __BIN_APP_H

#include "stm32f10x.h"

/* 智能垃圾桶应用层
 *
 * 上电调用顺序：
 *     BinApp_Init();      初始化传感器 / 舵机 / OLED / 串口 / 看门狗
 *     BinApp_Run();       状态机主循环，不返回
 */
void BinApp_Init(void);
void BinApp_Run(void);

#endif
