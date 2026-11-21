#include "stm32f10x.h"
#include "beep.h"
#include "BoardConfig.h"

/* 板载蜂鸣器：低电平响（极性见 BoardConfig.h 的 BOARD_BEEP_ACTIVE） */

static void beep_write(uint8_t on)
{
    BitAction level = on ? (BitAction)BOARD_BEEP_ACTIVE
                         : (BitAction)(!BOARD_BEEP_ACTIVE);
    GPIO_WriteBit(BOARD_BEEP_PORT, BOARD_BEEP_PIN, level);
}

void Beep_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = BOARD_BEEP_PIN;
    GPIO_Init(BOARD_BEEP_PORT, &gpio);

    Beep_Off();
}

void Beep_On(void)  { beep_write(1); }
void Beep_Off(void) { beep_write(0); }
