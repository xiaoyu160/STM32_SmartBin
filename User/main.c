#include "stm32f10x.h"
#include "BinApp.h"

int main(void)
{
    BinApp_Init();     
    BinApp_Run();    

    while (1)
    {
    }
}
