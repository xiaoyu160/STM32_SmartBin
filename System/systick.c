#include "stm32f10x.h"
#include "systick.h"

/* 1ms 系统时基
 *
 * 全工程的"当前时间"只有 sys_ms 这一个来源：中断里自增，
 * 主循环用 (sys_ms - 起点) >= 周期 判断超时，不做阻塞等待。
 * 无符号减法在溢出时依然正确，所以 49 天不用管。 */

volatile uint32_t sys_ms = 0;

void SysTick_Init(void)
{
    if (SysTick_Config(SystemCoreClock / 1000))
    {
        while (1);          /* 配置失败说明系统时钟不对，停在这里等调试器 */
    }
}

/* 基于 sys_ms 的阻塞延时，只能在 SysTick_Init() 之后调用。
 * 仅用于开机画面这类"还没进主循环、不需要并行处理"的场合。 */
void SysTick_DelayMs(uint32_t ms)
{
    uint32_t start = sys_ms;
    while ((sys_ms - start) < ms)
    {
    }
}
