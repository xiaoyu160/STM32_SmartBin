#include "stm32f10x.h"                  // Device header
#include "Encoder_TIM.h"
#include "OLED.h"
#include "Delay.h"

void Encoder_TIM_Init(void)
{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitTypeDef g = {0};
    g.GPIO_Mode = GPIO_Mode_IPU;            // A/B 相上拉输入
    g.GPIO_Pin  = GPIO_Pin_6 | GPIO_Pin_7;  // PA6=CH1(A), PA7=CH2(B)
    GPIO_Init(GPIOA, &g);

    TIM_TimeBaseInitTypeDef t = {0};
    t.TIM_Prescaler   = 0;
    t.TIM_Period      = 0xFFFF;             // 16位满量程
    t.TIM_CounterMode = TIM_CounterMode_Up; // 编码器模式下被接口配置接管
    TIM_TimeBaseInit(TIM3, &t);

    // 硬件正交解码：TI12 模式 + 双相边沿，自动判方向
    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12,
                               TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_Cmd(TIM3, ENABLE);
}

void Encoder_TIM_Demo(void)
{
    OLED_Init();
    Encoder_TIM_Init();
    OLED_Clear();
    OLED_ShowString(1, 1, "CNT:");
    OLED_ShowString(2, 1, "Delta:");
    OLED_ShowString(3, 1, "Total:");

    int16_t enc_last = 0;
    int32_t total = 0;

    while(1)
    {
        int16_t now   = TIM_GetCounter(TIM3);     // 有符号读，含方向
        int16_t delta = now - enc_last;            // ★ 增量法：自动处理回绕+方向
        enc_last = now;
        total += delta;

        OLED_ShowSignedNum(1, 6, now,   5);
        OLED_ShowSignedNum(2, 8, delta, 5);
        OLED_ShowSignedNum(3, 7, total, 6);
        Delay_ms(50);                              // 刷新节流
    }
}
