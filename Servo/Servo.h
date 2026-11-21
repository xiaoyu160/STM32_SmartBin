#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"
#include "BoardConfig.h"

/* ------------------------------------------------------------------
 * SG90 舵机驱动（50Hz PWM，0.5ms ~ 2.5ms 对应 0 ~ 180 度）
 *
 * 信号脚：PA8 = TIM1_CH1，见 BoardConfig.h
 * 供电  ：5V，且必须与开发板共地
 * ------------------------------------------------------------------ */

#define SERVO_TIM            TIM1
#define SERVO_TIM_CLK        RCC_APB2Periph_TIM1

#define SERVO_GPIO           BOARD_SERVO_PORT
#define SERVO_GPIO_CLK       RCC_APB2Periph_GPIOA
#define SERVO_PIN            BOARD_SERVO_PIN

#define SERVO_PRESCALER      (72 - 1)        /* 72MHz / 72 = 1MHz，1 计数 = 1us */
#define SERVO_PERIOD_US      20000u          /* 20ms -> 50Hz */
#define SERVO_MIN_US         500u            /* 0.5ms ->   0 度 */
#define SERVO_MAX_US         2500u           /* 2.5ms -> 180 度 */

void     Servo_Init(void);
void     Servo_SetPulseUs(uint16_t us);
void     Servo_SetAngle(int16_t angle);
uint16_t Servo_AngleToPulse(int16_t angle);

#endif
