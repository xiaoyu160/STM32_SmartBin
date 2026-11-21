#include "stm32f10x.h"
#include "BinApp.h"

int main(void)
{
    BinApp_Init();      /* 硬件初始化 + 开机画面，之后时基已经可用 */
    BinApp_Run();       /* 状态机主循环，正常情况不会返回 */

    return 0;
}
