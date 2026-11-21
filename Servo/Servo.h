#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f10x.h"

/* ------------------------------------------------------------------
 * SG90 servo driver  (50Hz PWM, 0.5ms ~ 2.5ms -> 0 ~ 180 degree)
 *
 * Signal pin : PA8  = TIM1_CH1
 *              (PA8 is free on this board: LED uses PA0-PA3,
 *               keys PA2/PA4/PA6, encoder PA6/PA7 + PA9/PA10,
 *               fan PB6, OLED PB8/PB9, UART PB10/PB11, IR PB14)
 *
 * Power      : 5V !  SG90 needs 4.8~6V, 3.3V is not enough to move it.
 *
 * Knob       : PA6 (A) / PA7 (B), TIM3 hardware encoder mode.
 *
 * If PA8 is already taken on your board, tell me and I will port the
 * driver to another free timer channel (e.g. PB7 / TIM4_CH2).
 * ------------------------------------------------------------------ */

#define SERVO_TIM            TIM1
#define SERVO_TIM_CLK        RCC_APB2Periph_TIM1

#define SERVO_GPIO           GPIOA
#define SERVO_GPIO_CLK       RCC_APB2Periph_GPIOA
#define SERVO_PIN            GPIO_Pin_8      /* TIM1_CH1 */

#define SERVO_PRESCALER      (72 - 1)        /* 72MHz / 72 = 1MHz, 1 count = 1us */
#define SERVO_PERIOD_US      20000u          /* 20ms -> 50Hz */
#define SERVO_MIN_US         500u            /* 0.5ms  ->   0 degree */
#define SERVO_MAX_US         2500u           /* 2.5ms  -> 180 degree */

/* Encoder counts per 1 degree of movement.
 * One mechanical detent usually gives 1 delta, so 2 means 2 degree/click.
 * Too fast / too slow? Just change this value. */
#define SERVO_DEG_PER_COUNT  2

void     Servo_Init(void);
void     Servo_SetPulseUs(uint16_t us);
void     Servo_SetAngle(int16_t angle);
uint16_t Servo_AngleToPulse(int16_t angle);

void     Servo_Test(void);   /* 0->180->0 sweep, use it to check wiring */
void     Servo_EndStopTest(void); /* 500us / 2500us / 1500us, 3s each */
void     Servo_Demo(void);   /* knob (encoder) controls the angle + OLED */

#endif
