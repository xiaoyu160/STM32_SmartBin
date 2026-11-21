#include "stm32f10x.h"
#include "UART.h"
#include "BoardConfig.h"

/* USART3 单向发送驱动（PB10 = TX，PB11 = RX）
 *
 * 本工程只需要"往电脑发日志"这一个方向，所以不打开接收中断：
 * 接收若要做命令解析，应该把字节收进环形缓冲、交给主循环处理，
 * 不要在中断里解析和回包（会拖住 SysTick，也会和主循环抢外设）。 */

void USART3_Init(uint32_t baud)
{
    GPIO_InitTypeDef  gpio;
    USART_InitTypeDef uart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;          /* TX */
    gpio.GPIO_Pin   = BOARD_UART_TX_PIN;
    GPIO_Init(BOARD_UART_PORT, &gpio);

    gpio.GPIO_Mode  = GPIO_Mode_IPU;            /* RX */
    gpio.GPIO_Pin   = BOARD_UART_RX_PIN;
    GPIO_Init(BOARD_UART_PORT, &gpio);

    uart.USART_BaudRate            = baud;
    uart.USART_WordLength          = USART_WordLength_8b;
    uart.USART_StopBits            = USART_StopBits_1;
    uart.USART_Parity              = USART_Parity_No;
    uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    uart.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &uart);
    USART_Cmd(USART3, ENABLE);
}

void USART3_SendString(const char *s)
{
    while (*s)
    {
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
        USART_SendData(USART3, (uint8_t)*s++);
    }
}
