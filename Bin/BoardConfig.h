#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include "stm32f10x.h"

/* ==================================================================
 * 硬件接线表 —— 全工程唯一的一份引脚定义
 *
 * 换板子、改接线、发现亮灭反了，只改这一个文件。
 * 驱动层（Led / Beep / Servo / Oled / Uart）和应用层都从这里取引脚。
 *
 * MCU : STM32F103C8T6  72MHz
 * ================================================================== */

/* ---------------- 数字输出 ---------------------------------------- */

/* 板载 LED（满溢指示）
 * 电平极性按实测填：本板是高电平点亮，所以 ACTIVE = 1。
 * 换板子后如果亮灭反了，把 1 改成 0 即可。 */
#define BOARD_LED_PORT          GPIOA
#define BOARD_LED_PIN           GPIO_Pin_0
#define BOARD_LED_ACTIVE        1

/* 板载蜂鸣器（低电平响） */
#define BOARD_BEEP_PORT         GPIOA
#define BOARD_BEEP_PIN          GPIO_Pin_4
#define BOARD_BEEP_ACTIVE       0

/* ---------------- 数字输入 ---------------------------------------- */

/* 手动按键（板载 KEY1，内部上拉，按下读到低电平） */
#define BOARD_KEY_PORT          GPIOA
#define BOARD_KEY_PIN           GPIO_Pin_2
#define BOARD_KEY_ACTIVE        0

/* 红外感应（投放口，检测手/物体靠近）
 *
 * 四脚模块（丝印 VCC GND DO AO，LM393 + 反射式探头）：
 *   VCC -> 5V   GND -> GND   DO -> PB14   AO 悬空不接
 *   （AO 是模拟量输出，电压可到 VCC，不要接到不耐 5V 的 ADC 脚）
 *
 * 若模块"检测到目标"时 DO 输出高电平，把 ACTIVE 改成 1，不用改硬件。
 * 三脚模块（E18-D80NK 等）同理：棕 VCC / 蓝 GND / 黑 OUT。 */
#define BOARD_IR_HAND_PORT      GPIOB
#define BOARD_IR_HAND_PIN       GPIO_Pin_14
#define BOARD_IR_HAND_ACTIVE    0       /* 0 = 检测到目标输出低；1 = 输出高 */

/* 红外对射（桶口横向对射，光路被垃圾挡住 = 满）
 *
 * 这是"发射端 + 接收端"两个器件，只有接收端有 OUT/DO。
 * 接线：接收端 OUT -> PB13，发射端只接 5V / GND。
 *
 * 有效电平怎么定：看 OLED 第 3 行的 RAW 值，用纸板挡住光路，
 *   挡光时 RAW=0 -> 这里填 0；挡光时 RAW=1 -> 这里填 1。
 * 填对了，挡光 1 秒后第 1 行才会变成【桶已满】。 */
#define BOARD_IR_FULL_PORT      GPIOB
#define BOARD_IR_FULL_PIN       GPIO_Pin_13
#define BOARD_IR_FULL_ACTIVE    1       /* 1 = 光路被挡输出高；0 = 输出低 */

/* ---------------- 复用功能 ---------------------------------------- */

/* 舵机 SG90 信号线：PA8 = TIM1_CH1，50Hz PWM，供电必须 5V */
#define BOARD_SERVO_PORT        GPIOA
#define BOARD_SERVO_PIN         GPIO_Pin_8

/* OLED（软件 I2C，开漏输出） */
#define BOARD_OLED_PORT         GPIOB
#define BOARD_OLED_SCL_PIN      GPIO_Pin_8
#define BOARD_OLED_SDA_PIN      GPIO_Pin_9

#endif
