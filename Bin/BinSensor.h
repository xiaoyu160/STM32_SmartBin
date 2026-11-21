#ifndef __BIN_SENSOR_H
#define __BIN_SENSOR_H

#include "stm32f10x.h"

/* 输入设备统一驱动：2 路红外 + 1 个按键
 *
 * 全部用"周期采样 + 计数去抖"，不用外部中断：
 *   - 中断里做业务容易和主循环抢数据，也不好调
 *   - 10ms 采样一次，40ms 去抖，对垃圾桶这种慢动作足够
 *
 * 用法：上电 BinSensor_Init()；主循环每 10ms 调一次 BinSensor_Update()，
 *       之后随时读 BinSensor_Hand() / BinSensor_Full() / BinSensor_KeyPressed()。
 */
void    BinSensor_Init(void);
void    BinSensor_Update(void);     /* 必须每 10ms 调一次 */

uint8_t BinSensor_Hand(void);       /* 1 = 投放口检测到手/物体 */
uint8_t BinSensor_Full(void);       /* 1 = 对射光路被挡住（垃圾到口） */
uint8_t BinSensor_KeyPressed(void); /* 1 = 本次扫描出现"按下"边沿，读后自动清零 */
uint8_t BinSensor_RawFull(void);    /* 未去抖的电平，调试用 */

#endif
