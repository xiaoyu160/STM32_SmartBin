#ifndef __BIN_CONFIG_H
#define __BIN_CONFIG_H

#include "BoardConfig.h"

/* ==================================================================
 * 智能垃圾桶 —— 业务参数
 *
 * 这里只放"想改一下试试"的东西：动作角度、保持时间、去抖时序、功能开关。
 * 引脚和有效电平在 BoardConfig.h，两者分开，改硬件不用翻业务参数。
 *
 * 控制方式（本版本只有全自动，没有开关可切）：
 *   红外感应到人 -> 舵机 90° 开盖 -> 保持 3 秒自动关盖
 *   按下 KEY1    -> 强制开盖 3 秒后自动关盖
 *   对射被挡 1 秒 -> 满溢报警（LED 常亮 + 蜂鸣器间歇 + OLED【桶已满】）
 * ================================================================== */

/* ---------------- 动作参数 ---------------------------------------- */
#define BIN_SERVO_CLOSED_DEG    0       /* 关盖角度 */
#define BIN_SERVO_OPEN_DEG      90      /* 开盖角度 */

/* 开盖后保持时间：从开盖那一刻算起，满 3 秒就关，不管手还在不在。
 * 手一直停在感应区时，盖子关上后会"锁定"，必须等手离开才允许再开，
 * 所以不会出现反复开合。 */
#define BIN_LID_HOLD_MS         3000u   /* 开盖后保持时间 = 3 秒 */

/* 防夹手模式：
 *   0 = 严格 3 秒（默认，符合"延时 3 秒自动关盖"的要求）
 *   1 = 手不离开就一直续期，离开后再等 3 秒才关（栏杆式垃圾桶的手感） */
#define BIN_LID_KEEP_OPEN_WHILE_HAND 0
#define BIN_LID_RETRIGGER_MS    1000u   /* 仅上面为 1 时生效：续期的最小间隔 */

/* ---------------- 时序 / 滤波参数 --------------------------------- */
#define BIN_LOOP_MS             10u     /* 主循环调度周期 */
#define BIN_IR_DEBOUNCE_MS      40u     /* 红外电平去抖时间 */
#define BIN_KEY_DEBOUNCE_MS     20u     /* 按键去抖时间 */
#define BIN_FULL_CONFIRM_MS     1000u   /* 光路连续遮挡多久判定为"满" */
#define BIN_FULL_CLEAR_MS       2000u   /* 光路恢复多久后解除"满"（清空垃圾） */
#define BIN_THROW_MIN_MS        50u     /* 光路遮挡超过这个时长才算"投了一件" */
#define BIN_BEEP_ON_MS          200u    /* 报警：响 200ms */
#define BIN_BEEP_OFF_MS         800u    /* 报警：停 800ms（间歇） */
#define BIN_UI_REFRESH_MS       100u    /* OLED 最小刷新间隔 */
#define BIN_BOOT_SPLASH_MS      800u    /* 开机画面停留时间 */

/* 去抖所需的采样次数（由上面的时间换算，别手改） */
#define BIN_IR_DEBOUNCE_TICKS   (BIN_IR_DEBOUNCE_MS  / BIN_LOOP_MS)
#define BIN_KEY_DEBOUNCE_TICKS  (BIN_KEY_DEBOUNCE_MS / BIN_LOOP_MS)

/* ---------------- 功能开关 ---------------------------------------- */
#define BIN_USE_UART            0       /* 串口上报（PB10/PB11, 115200），不用就 0 */
#define BIN_USE_IWDG            1       /* 独立看门狗，1 秒不喂狗就复位 */
#define BIN_USE_IDLE_SLEEP      0       /* 空闲进 Sleep，默认关 */
#define BIN_UART_BAUD           115200u
#define BIN_TELEMETRY_MS        5000u   /* 心跳上报周期 */

/* 上电舵机自检：设为 1 时，开机画面结束后舵机会 0°->90°->0° 摆一次，
 * 用来确认"舵机 + PWM + 供电"这条通路是好的。
 * 现象是盖子开一下又关上；正式使用请设回 0。 */
#define BIN_SERVO_SELFTEST      0

/* ---------------- OLED 汉字索引（OLED_Font.h 中的顺序） ------------ */
#define BIN_HZ_DAI              4       /* 待 */
#define BIN_HZ_JI               5       /* 机 */
#define BIN_HZ_KAI              6       /* 开 */
#define BIN_HZ_GAI              7       /* 盖 */
#define BIN_HZ_TONG             8       /* 桶 */
#define BIN_HZ_YI               9       /* 已 */
#define BIN_HZ_MAN              10      /* 满 */

#endif
