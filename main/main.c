#include <stdio.h>
#include "Inf/Inf_DBR6120.h"
#include "Inf/Inf_WTN6170.h"

void app_main(void)
{
    // 初始化电机模块
    Inf_DBR6120_Init();

    // Inf_DBR6120_Forward();
    // vTaskDelay(1000);

    // 开锁
    Inf_DBR6120_OpenLock();

    // 初始化语音模块
    Inf_WTN6170_Init();

    // 发送指令
    Inf_WTN6170_SendCmd(0xF3);
    Inf_WTN6170_SendCmd(64);
    Inf_WTN6170_SendCmd(0xF3);
    Inf_WTN6170_SendCmd(65);

    while (1)
    {
        vTaskDelay(100);
    }
}
