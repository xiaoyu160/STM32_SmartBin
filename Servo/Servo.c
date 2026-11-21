#include "stm32f10x.h"
#include "Servo.h"

/* ------------------------------------------------------------------
 * SG90 舵机驱动（PA8 / TIM1_CH1）
 *
 * TIM1 由 72MHz 驱动：
 *   PSC = 71        -> 计数频率 1MHz，1 个计数 = 1us
 *   ARR = 19999     -> 20000us = 20ms = 50Hz
 *   CCR = 500..2500 -> 高电平 0.5ms..2.5ms
 *
 * 舵机供电必须 5V（4.8~6V）。3.3V 带不动，只会发出嗡嗡声。
 * 舵机地必须和开发板共地。
 * ------------------------------------------------------------------ */

void Servo_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(SERVO_GPIO_CLK | SERVO_TIM_CLK, ENABLE);

    /* 信号脚配成复用推挽（TIM1_CH1） */
    GPIO_InitStructure.GPIO_Pin   = SERVO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERVO_GPIO, &GPIO_InitStructure);

    /* 50Hz 时基 */
    TIM_TimeBaseStructure.TIM_Prescaler         = SERVO_PRESCALER;
    TIM_TimeBaseStructure.TIM_Period            = SERVO_PERIOD_US - 1;
    TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(SERVO_TIM, &TIM_TimeBaseStructure);

    /* 通道 1，PWM 模式 1 */
    TIM_OCInitStructure.TIM_OCMode       = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState  = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStructure.TIM_Pulse        = SERVO_MIN_US;
    TIM_OCInitStructure.TIM_OCPolarity   = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity  = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState  = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(SERVO_TIM, &TIM_OCInitStructure);

    /* CCR 在更新事件时装载，避免改角度瞬间出现窄脉冲 */
    TIM_OC1PreloadConfig(SERVO_TIM, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(SERVO_TIM, ENABLE);

    TIM_Cmd(SERVO_TIM, ENABLE);

    /* TIM1 是高级定时器：必须打开主输出，否则引脚一直是低电平 */
    TIM_CtrlPWMOutputs(SERVO_TIM, ENABLE);
}

/* us: 脉宽（微秒），钳在 500..2500 */
void Servo_SetPulseUs(uint16_t us)
{
    if (us < SERVO_MIN_US) us = SERVO_MIN_US;
    if (us > SERVO_MAX_US) us = SERVO_MAX_US;
    TIM_SetCompare1(SERVO_TIM, us);
}

/* angle: 0..180 度，超范围自动钳位 */
void Servo_SetAngle(int16_t angle)
{
    Servo_SetPulseUs(Servo_AngleToPulse(angle));
}

uint16_t Servo_AngleToPulse(int16_t angle)
{
    if (angle < 0)   angle = 0;
    if (angle > 180) angle = 180;
    return (uint16_t)(SERVO_MIN_US +
           (uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US) / 180u);
}
