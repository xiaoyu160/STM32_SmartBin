#ifndef __LED_H
#define __LED_H

#define LED_PORT GPIOA
#define LED_PIN_0 GPIO_Pin_0
#define LED_PIN_1 GPIO_Pin_1
#define LED_PIN_2 GPIO_Pin_2
#define LED_PIN_3 GPIO_Pin_3

void LED_INIT(void);
void led_flowing_light(void);
void TIM2_Init(void);
void LED_TIM_Demo(void);

#endif
