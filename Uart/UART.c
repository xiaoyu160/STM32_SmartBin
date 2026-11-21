#include "stm32f10x.h"
#include "string.h"
#include "UART.h"
#include "LED.h"
#include "OLED.h"
#include "beep.h"

// usart.c —— 初始化 USART3 (PB10=TX, PB11=RX)
void USART3_Init(uint32_t baud)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_InitTypeDef g;
    g.GPIO_Mode = GPIO_Mode_AF_PP;  g.GPIO_Pin = GPIO_Pin_10; g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &g);                       // TX
    g.GPIO_Mode = GPIO_Mode_IPU;  g.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOB, &g);                       // RX

    USART_InitTypeDef u = {0};
    u.USART_BaudRate = baud; u.USART_WordLength = USART_WordLength_8b;
    u.USART_StopBits = USART_StopBits_1; u.USART_Parity = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &u);
    USART_Cmd(USART3, ENABLE);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    NVIC_InitTypeDef n = {0};
    n.NVIC_IRQChannel = USART3_IRQn; n.NVIC_IRQChannelPreemptionPriority = 1;
    n.NVIC_IRQChannelSubPriority = 1; n.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&n);
}

void USART3_SendString(char *s)
{
	while(*s)
	{
		USART_SendData(USART3,(uint8_t)*s++);
    while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);
	} 
}

		
// 命令解析 + 控制（低电平点亮）
uint8_t led_state[4] = {0,0,0,0};

void Parse_Cmd(char *cmd)
{
    if (strncmp(cmd, "LED", 3) == 0)          // LED_ON / LED_OFF / LED_TG  → PA0
    {
        char *a = cmd + 4;                    // 跳过 "LED_"
        if      (strncmp(a, "ON", 2)  == 0) GPIO_ResetBits(LED_PORT, LED_PIN_0); // 低电平亮
        else if (strncmp(a, "OFF", 3) == 0) GPIO_SetBits(LED_PORT, LED_PIN_0);
        else if (strncmp(a, "TG", 2)  == 0)
        {
            if (GPIO_ReadOutputDataBit(LED_PORT, LED_PIN_0) == Bit_SET)
                GPIO_ResetBits(LED_PORT, LED_PIN_0);
            else GPIO_SetBits(LED_PORT, LED_PIN_0);
        }
        else { USART3_SendString("ERR\r\n"); return; }
    }
    else if (strncmp(cmd, "BEEP", 4) == 0)    // BEEP_ON / BEEP_OFF / BEEP_TG
    {
        char *a = cmd + 5;                    // 跳过 "BEEP_"
        if      (strncmp(a, "ON", 2)  == 0) Beep_On();
        else if (strncmp(a, "OFF", 3) == 0) Beep_Off();
        else if (strncmp(a, "TG", 2)  == 0) Beep_Toggle();
        else { USART3_SendString("ERR\r\n"); return; }
    }
    else { USART3_SendString("ERR\r\n"); return; }
    USART3_SendString("OK\r\n");
}
	
// 接收中断 + 行缓冲（放在 stm32f10x_it.c 的 USART3_IRQHandler）
static char rx_buf[64]; static uint8_t rx_idx = 0;
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        uint8_t c = USART_ReceiveData(USART3);
        if (c=='\n' || c=='\r') {
            if (rx_idx) { rx_buf[rx_idx]='\0'; Parse_Cmd(rx_buf); rx_idx=0; }
        } else if (rx_idx < 63) rx_buf[rx_idx++] = c;
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

void Uart_Demo(void)
{
		LED_INIT();
    Beep_Init();
    USART3_Init(115200);
    OLED_Init();
    OLED_Clear();

    OLED_ShowString(1, 1, "LED+BEEP Demo");     // 13 字符
    OLED_ShowString(2, 1, "LED_ ON/OFF/TG");    // 14 字符
    OLED_ShowString(3, 1, "BEEP_ ON/OFF/TG");   // 15 字符

    char rx_buf[32];
    uint8_t idx = 0;

    while (1)
    {
        if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) == SET)
        {
            uint8_t c = USART_ReceiveData(USART3);
            if (c == '\r' || c == '\n')
            {
                if (idx > 0) { rx_buf[idx] = '\0'; Parse_Cmd(rx_buf); idx = 0; }
            }
            else if (idx < 31) rx_buf[idx++] = c;
        }

        /* 第4行刷新状态：L 为 LED(0灭1亮)，B 为蜂鸣器(0停1响) */
        uint8_t led  = (GPIO_ReadOutputDataBit(LED_PORT,  LED_PIN_0) == Bit_RESET) ? 1 : 0;
        uint8_t beep = (GPIO_ReadOutputDataBit(BEEP_PORT, BEEP_PIN)  == Bit_SET)   ? 1 : 0;
        OLED_ShowString(4, 1, "L:");
        OLED_ShowChar(4, 3, '0' + led);
        OLED_ShowString(4, 5, "B:");
        OLED_ShowChar(4, 7, '0' + beep);
    }
}
