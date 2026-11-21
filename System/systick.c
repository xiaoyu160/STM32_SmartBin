#include "stm32f10x.h"
#include "systick.h"
#include "LED.h"

volatile uint32_t sys_ms = 0;
volatile uint16_t tick_cnt = 0;
volatile uint8_t flag_10ms = 0;
volatile uint8_t flag_100ms = 0;
volatile uint8_t flag_1s = 0;

void SysTick_Init(void)
{
	if(SysTick_Config(SystemCoreClock / 1000))
	{
		while(1);
	}
}

void SysTick_Demo(void)
{
	LED_INIT();
	SysTick_Init();
	
	while (1)
	{
		if(flag_10ms == 1)
		{
			GPIO_WriteBit(LED_PORT, LED_PIN_0, (BitAction)!GPIO_ReadOutputDataBit(LED_PORT, LED_PIN_0));
//			GPIO_ResetBits(LED_PORT, LED_PIN_0);
			flag_10ms = 0;
		}
		if(flag_100ms == 1)
		{
			GPIO_WriteBit(LED_PORT, LED_PIN_1, (BitAction)!GPIO_ReadOutputDataBit(LED_PORT, LED_PIN_1));
//			GPIO_ResetBits(LED_PORT, LED_PIN_1);
			flag_100ms = 0;
		}
		if(flag_1s == 1)
		{
			GPIO_WriteBit(LED_PORT, LED_PIN_2, (BitAction)!GPIO_ReadOutputDataBit(LED_PORT, LED_PIN_2));
//			GPIO_ResetBits(LED_PORT, LED_PIN_2);
			flag_1s = 0;
		}
	}
}
