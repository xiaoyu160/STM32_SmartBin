#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "OLED.h"
#include "systick.h"

void LED_INIT(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = LED_PIN_0 | LED_PIN_1 | LED_PIN_2 | LED_PIN_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);
	
	GPIO_WriteBit(LED_PORT, LED_PIN_0, (BitAction)1);
	GPIO_WriteBit(LED_PORT, LED_PIN_1, (BitAction)1);
	GPIO_WriteBit(LED_PORT, LED_PIN_2, (BitAction)1);
	GPIO_WriteBit(LED_PORT, LED_PIN_3, (BitAction)1);
}

void TIM2_Init(void)
{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInitTypeDef t = {0};
    t.TIM_Prescaler     = 7200 - 1;       // 72MHz/7200 = 10kHz
    t.TIM_Period        = 5000 - 1;       // 10kHz/5000 = 500ms 进一次中断
    t.TIM_CounterMode   = TIM_CounterMode_Up;
    t.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &t);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    NVIC_InitTypeDef n = {0};
    n.NVIC_IRQChannel = TIM2_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 1;
    n.NVIC_IRQChannelSubPriority = 1;
    n.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&n);
}

void TIM2_PWM_Init(void)
{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitTypeDef g = {0};
    g.GPIO_Mode = GPIO_Mode_AF_PP;     // PA0 复用推挽 = TIM2_CH1
    g.GPIO_Pin = GPIO_Pin_0;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    TIM_TimeBaseInitTypeDef t = {0};
    t.TIM_Prescaler = 72 - 1;          // 72MHz/72 = 1MHz
    t.TIM_Period    = 1000 - 1;        // 1MHz/1000 = 1kHz PWM
    t.TIM_CounterMode = TIM_CounterMode_Up;
    t.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &t);

    TIM_OCInitTypeDef oc = {0};
    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OCPolarity = TIM_OCPolarity_Low;   // 有效电平为低 → ccr 越大越亮（LED 低电平亮）
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse = 0;
    TIM_OC1Init(TIM2, &oc);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_Cmd(TIM2, ENABLE);
}

// PA0~PA3四个LED灯流水灯
void led_flowing_light(void)
{
	LED_INIT();
	
	// 引脚数组
	static const uint16_t pins[4] = {LED_PIN_0, LED_PIN_1, LED_PIN_2, LED_PIN_3};
	
	while (1)
	{
		for(int i=0; i<4; i++)
		{
			GPIO_ResetBits(LED_PORT, pins[i]);
			Delay_ms(500);
			GPIO_SetBits(LED_PORT, pins[i]);
			Delay_ms(500);
		}
		
//		GPIO口设置电平的另外写法
//		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
//		Delay_ms(500);
//		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
//		Delay_ms(500);
//		
//		GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)0);
//		Delay_ms(500);
//		GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)1);
//		Delay_ms(500);
	}
}

void LED_TIM_Demo(void)
{
	LED_INIT();
	TIM2_PWM_Init();
	OLED_Init();
	SysTick_Init();
	
	OLED_ShowString(1, 1, "Duty: ");
	OLED_ShowString(2, 1, "Stage: ");
	
		uint16_t ccr = 0;
    int8_t   dir = 1;

    while (1)
    {
        if (flag_10ms == 1)              // 每 10ms 步进一次占空比
        {
            flag_10ms = 0;
            ccr += dir * 5;             // 步长 10（ARR=1000）
            if (ccr >= 1000) { ccr = 1000; dir = -1; }
            if (ccr == 0)    { dir = 1; }
            TIM_SetCompare1(TIM2, ccr);  // 更新占空比（硬件自动输出）

            OLED_ShowNum(1, 7, ccr / 10, 3);                    // 亮度 0~100%
            OLED_ShowString(2, 8, dir > 0 ? "RISE" : "FALL");   // 阶段
        }
    }
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        GPIO_WriteBit(LED_PORT, LED_PIN_0, (BitAction)!GPIO_ReadOutputDataBit(LED_PORT, LED_PIN_0));   // 直接翻转 PA0
    }
}
