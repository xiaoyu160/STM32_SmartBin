#include "stm32f10x.h"                  // Device header
#include "Encoder.h"
#include "OLED.h"
#include "Delay.h"

void Encoder_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	/*A相——PA9*/
	/*B相——PA10*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource9);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line9;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	// 上升沿中断
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

static volatile int16_t enc_cnt = 0;
static int16_t last_cnt = 0;

void EXTI9_5_IRQHandler(void)
{
	if (EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        /* 读 B 相（PA10）判断方向；方向反了就交换 ++/-- */
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10) == Bit_SET)
            enc_cnt--;
        else
            enc_cnt++;

        EXTI_ClearITPendingBit(EXTI_Line9);  /* 必须清标志，否则会一直进中断 */
    }
}

int16_t Encoder_GetDelta(void)
{
    int16_t now   = enc_cnt;
    int16_t delta = now - last_cnt;
    last_cnt = now;
    return delta;
}

void Encoder_Demo(void)
{
	OLED_Init();
	Encoder_Init();
	
	 int16_t delta;
   int32_t total      = 0;
   int32_t last_total = -1;   /* 保证首次一定刷新 */

   OLED_Clear();
   OLED_ShowString(1, 1, "Delta: ");
   OLED_ShowString(2, 1, "Total: ");

   while (1)
   {
		 delta = Encoder_GetDelta();
     if (delta != 0)
			 total += delta;
     if (total != last_total)
     {
       last_total = total;
       OLED_ShowSignedNum(1, 8,  delta, 5);
       OLED_ShowSignedNum(2, 8, total, 5);
     }
     Delay_ms(10);
   }
}
