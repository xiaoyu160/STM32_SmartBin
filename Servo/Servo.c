#include "stm32f10x.h"
#include "Servo.h"
#include "Encoder_TIM.h"
#include "OLED.h"
#include "Delay.h"

/* ------------------------------------------------------------------
 * SG90 servo on PA8 / TIM1_CH1
 *
 * TIM1 runs from 72MHz:
 *   PSC = 71        -> 1MHz counter, 1 count = 1us
 *   ARR = 19999     -> 20000us = 20ms period = 50Hz
 *   CCR = 500..2500 -> 0.5ms..2.5ms high level
 *
 * Servo power MUST be 5V (4.8~6V). 3.3V is not enough, the motor will
 * not move (it may just buzz). Ground must be common with the board.
 * ------------------------------------------------------------------ */

void Servo_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef       TIM_OCInitStructure;

    RCC_APB2PeriphClockCmd(SERVO_GPIO_CLK | SERVO_TIM_CLK, ENABLE);

    /* PA8 as alternate function push-pull (TIM1_CH1) */
    GPIO_InitStructure.GPIO_Pin   = SERVO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SERVO_GPIO, &GPIO_InitStructure);

    /* 50Hz time base */
    TIM_TimeBaseStructure.TIM_Prescaler         = SERVO_PRESCALER;
    TIM_TimeBaseStructure.TIM_Period            = SERVO_PERIOD_US - 1;
    TIM_TimeBaseStructure.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(SERVO_TIM, &TIM_TimeBaseStructure);

    /* Channel 1, PWM mode 1 */
    TIM_OCInitStructure.TIM_OCMode       = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState  = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStructure.TIM_Pulse        = SERVO_MIN_US;
    TIM_OCInitStructure.TIM_OCPolarity   = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity  = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState  = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(SERVO_TIM, &TIM_OCInitStructure);

    /* keep CCR value latched at update event, avoids glitches */
    TIM_OC1PreloadConfig(SERVO_TIM, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(SERVO_TIM, ENABLE);

    TIM_Cmd(SERVO_TIM, ENABLE);

    /* TIM1 is an advanced timer: main output must be enabled, otherwise
       the pin stays low and the servo will not move */
    TIM_CtrlPWMOutputs(SERVO_TIM, ENABLE);
}

/* us: pulse width in microseconds, clamped to 500..2500 */
void Servo_SetPulseUs(uint16_t us)
{
    if (us < SERVO_MIN_US) us = SERVO_MIN_US;
    if (us > SERVO_MAX_US) us = SERVO_MAX_US;
    TIM_SetCompare1(SERVO_TIM, us);
}

/* angle: 0..180 degree, clamped */
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

/* ------------------------------------------------------------------
 * Sweep test: 0 -> 180 -> 0, no encoder / OLED needed.
 * Call it from main() to verify the servo wiring and its 5V supply.
 * ------------------------------------------------------------------ */
void Servo_Test(void)
{
    int16_t a;

    Servo_Init();

    while (1)
    {
        for (a = 0; a <= 180; a += 5)
        {
            Servo_SetAngle(a);
            Delay_ms(50);
        }
        for (a = 180; a >= 0; a -= 5)
        {
            Servo_SetAngle(a);
            Delay_ms(50);
        }
    }
}

/* ------------------------------------------------------------------
 * End stop test: 0.5ms -> 2.5ms -> 1.5ms, 3 seconds each.
 * If the servo can not even reach an end stop with no load, the problem
 * is not the code: it is supply current, a jammed gear box or a dead
 * servo.
 * ------------------------------------------------------------------ */
void Servo_EndStopTest(void)
{
    Servo_Init();

    while (1)
    {
        Servo_SetPulseUs(SERVO_MIN_US);   /* 0   degree */
        Delay_ms(3000);
        Servo_SetPulseUs(SERVO_MAX_US);   /* 180 degree */
        Delay_ms(3000);
        Servo_SetPulseUs(1500);           /* 90  degree */
        Delay_ms(3000);
    }
}

/* ------------------------------------------------------------------
 * Experiment: turn the knob, servo follows, angle on OLED
 *
 *   servo  : PA8  / TIM1_CH1
 *   encoder: PA6 (A) / PA7 (B) -> TIM3 hardware encoder mode, 4x count
 *            (the knob is soldered here, see Encoder_TIM.c)
 *   OLED   : PB8 (SCL) / PB9 (SDA)
 *
 * One mechanical detent gives 4 hardware counts, so we divide by 4 and
 * keep the remainder: slow turning still accumulates correctly.
 * ------------------------------------------------------------------ */
void Servo_Demo(void)
{
    int16_t  a;
    int16_t  now, dcnt, steps;
    int16_t  angle      = 90;
    int16_t  last_angle = -1;
    int16_t  last_cnt;
    int16_t  rem        = 0;      /* leftover counts (< 4)      */
    int32_t  detents    = 0;      /* total clicks, for display  */
    uint16_t pulse;
    uint16_t tick = 0;
    RCC_ClocksTypeDef clocks;
    uint8_t  clk_mhz;

    Servo_Init();
    Encoder_TIM_Init();
    OLED_Init();
    OLED_Clear();

    RCC_GetClocksFreq(&clocks);
    clk_mhz = (uint8_t)(clocks.PCLK2_Frequency / 1000000u);

    /* Step 1: sweep once, so we can see whether PWM really arrives */
    OLED_ShowString(1, 1, "Self test...");
    for (a = 0; a <= 180; a += 10) { Servo_SetAngle(a); Delay_ms(20); }
    for (a = 180; a >= 0; a -= 10) { Servo_SetAngle(a); Delay_ms(20); }

    OLED_Clear();
    Servo_SetAngle(angle);
    last_cnt = TIM_GetCounter(TIM3);

    while (1)
    {
        /* ---- read TIM3 quadrature counter, 4 counts per detent ---- */
        now  = (int16_t)TIM_GetCounter(TIM3);
        dcnt = now - last_cnt;
        last_cnt = now;

        rem += dcnt;
        steps = (int16_t)(rem / 4);
        rem   = (int16_t)(rem - steps * 4);

        if (steps != 0)
        {
            detents += steps;
            angle += steps * SERVO_DEG_PER_COUNT;
            if (angle < 0)   angle = 0;
            if (angle > 180) angle = 180;
            Servo_SetAngle(angle);
        }

        if (angle != last_angle)
        {
            last_angle = angle;
            pulse = Servo_AngleToPulse(angle);

            OLED_ShowString(1, 1, "Angle:");
            OLED_ShowNum(1, 8, (uint32_t)angle, 3);
            OLED_ShowString(1, 12, "deg");

            OLED_ShowString(2, 1, "Pulse:");
            OLED_ShowNum(2, 8, pulse, 4);
            OLED_ShowString(2, 13, "us");
        }

        /* ---- debug: click counter + raw A/B levels, 5 times/second ---- */
        if (++tick >= 100)
        {
            tick = 0;

            OLED_ShowString(3, 1, "Knob:");
            OLED_ShowSignedNum(3, 7, (int32_t)detents, 5);

            /* sanity check: APB2 frequency = TIM1 clock, must be 72MHz.
               If it shows 8MHz the HSE did not start, the PWM is then not
               50Hz and the servo will only buzz. */
            OLED_ShowString(4, 1, "CLK:");
            OLED_ShowNum(4, 5, (uint32_t)clk_mhz, 2);
            OLED_ShowString(4, 8, "MHz");
        }

        Delay_ms(2);
    }
}
