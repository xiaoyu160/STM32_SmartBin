#ifndef __BIN_APP_H
#define __BIN_APP_H

#include "stm32f10x.h"

/* 智能垃圾桶应用层
 *
 * 上电调用顺序：
 *     BinApp_Init();      传感器 / 舵机 / 蜂鸣器 / LED / 时基 / 开机画面 / 看门狗
 *     BinApp_Run();       状态机主循环，不返回
 *
 * 相关的文件：
 *     BinSensor.c     传感器采样与去抖
 *     BinUi.c         只负责画屏，数据由 BinUiFrame_t 传进去
 *     BinConfig.h     业务参数；引脚见 BoardConfig.h
 */
void BinApp_Init(void);
void BinApp_Run(void);

#endif
