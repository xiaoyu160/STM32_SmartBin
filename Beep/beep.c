#include "stm32f10x.h"
#include "beep.h"
#include "key.h"

void Beep_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Pin   = BEEP_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BEEP_PORT, &GPIO_InitStructure);
	
	GPIO_SetBits(BEEP_PORT, BEEP_PIN);
}

void Beep_On(void)
{
    GPIO_ResetBits(BEEP_PORT, BEEP_PIN);                 // 高电平响（按电路可反转）
}

void Beep_Off(void)
{
    GPIO_SetBits(BEEP_PORT, BEEP_PIN);
}

void Beep_Toggle(void)
{
    if (GPIO_ReadOutputDataBit(BEEP_PORT, BEEP_PIN) == Bit_SET)
        GPIO_ResetBits(BEEP_PORT, BEEP_PIN);
    else
        GPIO_SetBits(BEEP_PORT, BEEP_PIN);
}

void Beep_Demo(void)
{
	KEY_INIT();
	Beep_Init();
	
	while(1)
	{
		int key = key_scan();
		if(key == 3)
		{
			GPIO_ResetBits(BEEP_PORT, BEEP_PIN);
		}
	}
}
