#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "key.h"
#include "LED.h"

void KEY_INIT(void)
{
//  ¹Ø±Õ JTAG
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin   = KEY1_PIN | KEY2_PIN | KEY3_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

int key_scan(void)
{
	static const uint16_t pins[3] = { KEY1_PIN, KEY2_PIN, KEY3_PIN };
	static uint8_t key_state[3] = { 1, 1, 1 };
	static const int key_val[3] = { 1, 2, 3 };
	
	for(int i=0; i<3; i++)
	{
		if(key_state[i] && GPIO_ReadInputDataBit(KEY_PORT, pins[i]) == 0)
		{
			Delay_ms(50);
			if(GPIO_ReadInputDataBit(KEY_PORT, pins[i]) == 0)
			{
				key_state[i] = 0;
				return key_val[i];
			}
		}
		else if(GPIO_ReadInputDataBit(KEY_PORT, pins[i]) == 1)
		{
			key_state[i] = 1;
		}
	}
	return 0;
}

void key_demo(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef g = {0};
	g.GPIO_Mode = GPIO_Mode_Out_PP;  g.GPIO_Speed = GPIO_Speed_50MHz;
	g.GPIO_Pin  = LED_PIN_0 | LED_PIN_1 | LED_PIN_3;
	GPIO_Init(GPIOA, &g);
	KEY_INIT();
	
	static const uint16_t led_pins[3] = { LED_PIN_0, LED_PIN_1, LED_PIN_3 };
	
	while(1)
	{
		int key = key_scan();
		if(key == 1)
		{
			if(GPIO_ReadOutputDataBit(LED_PORT, led_pins[0])) GPIO_ResetBits(LED_PORT, led_pins[0]);
			else GPIO_SetBits(LED_PORT, led_pins[0]);
		}
		else if(key == 2)
		{
			if(GPIO_ReadOutputDataBit(LED_PORT, led_pins[1])) GPIO_ResetBits(LED_PORT, led_pins[1]);
			else GPIO_SetBits(LED_PORT, led_pins[1]);
		}
		else if(key == 3)
		{
			if(GPIO_ReadOutputDataBit(LED_PORT, led_pins[2])) GPIO_ResetBits(LED_PORT, led_pins[2]);
			else GPIO_SetBits(LED_PORT, led_pins[2]);
		}
	}
}
