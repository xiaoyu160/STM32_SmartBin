#ifndef __BIN_UI_H
#define __BIN_UI_H

#include "stm32f10x.h"

/* OLED 界面层
 *
 * 只负责"把一帧数据画到屏上"，不持有任何状态、不改业务逻辑。
 * 应用层每 100ms 填一次 BinUiFrame_t 传进来即可。
 *
 *   第 1 行：【待机】/【开盖】/【桶已满】+ 英文补充
 *   第 2 行：投放次数 / 开盖次数
 *   第 3 行：两路红外原始状态（接线自检就看这行）
 *   第 4 行：关盖倒计时 / 请清空 / 按键提示 */

typedef struct
{
    uint8_t  lid_open;          /* 1 = 盖子打开 */
    uint8_t  full;              /* 1 = 桶已满 */
    uint8_t  hand;              /* 投放口红外（已去抖） */
    uint8_t  raw_full;          /* 对射引脚原始电平 */
    uint32_t open_cnt;          /* 累计开盖次数 */
    uint32_t throw_cnt;         /* 累计投放次数 */
    uint32_t close_left_ms;     /* 关盖剩余时间，0 = 不显示倒计时 */
} BinUiFrame_t;

void BinUi_Splash(void);                    /* 开机画面，内部等待约 800ms */
void BinUi_Clear(void);
void BinUi_Render(const BinUiFrame_t *f);

#endif
