#include "stm32f10x.h"
#include "LED.h"
#include "BoardConfig.h"

/* 板载 LED（满溢指示）
 *
 * 只操作 BOARD_LED_PIN 这一根脚。这块板子的 PA1~PA3 还接着别的东西
 * （PA2 是手动按键），把整组 PA0~PA3 配成输出会把按键按死。
 * 极性由 BOARD_LED_ACTIVE 决定，硬件换板只改 BoardConfig.h。 */

void LED_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin   = BOARD_LED_PIN;
    GPIO_Init(BOARD_LED_PORT, &gpio);

    LED_Set(0);
}

void LED_Set(uint8_t on)
{
    BitAction level = on ? (BitAction)BOARD_LED_ACTIVE
                         : (BitAction)(!BOARD_LED_ACTIVE);
    GPIO_WriteBit(BOARD_LED_PORT, BOARD_LED_PIN, level);
}
