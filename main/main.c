#include <stdio.h>
#include "Inf/Inf_DBR6120.h"
#include "Inf/Inf_WTN6170.h"
#include "Inf/Inf_SC12B.h"
#include "Com_Debug.h"

extern uint8_t isTouch;

void app_main(void)
{
    // 初始化电机模块
    Inf_DBR6120_Init();

    // Inf_DBR6120_Forward();
    // vTaskDelay(1000);

    // // 开锁
    // Inf_DBR6120_OpenLock();

    // // 初始化语音模块
    // Inf_WTN6170_Init();

    // // 发送指令
    // Inf_WTN6170_SendCmd(0xF3);
    // Inf_WTN6170_SendCmd(64);
    // Inf_WTN6170_SendCmd(0xF3);
    // Inf_WTN6170_SendCmd(65);

    // 初始化SC12B按键模块

    Inf_SC12B_Init();
    Touch_Key touchKey = KEY_NO;

    while (1)
    {
        // touchKey = Inf_SC12B_ReadKey();
        // if (touchKey != KEY_NO)
        // {
        //     MY_LOGE("touchKey = %d", touchKey);
        // }

        if (isTouch)
        {
            touchKey = Inf_SC12B_ReadKey();
            MY_LOGE("touchKey = %d", touchKey);

            // 清除标记位
            isTouch = 0;
        }
        vTaskDelay(10);
    }
}
