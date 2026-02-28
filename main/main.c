#include <stdio.h>
#include "Inf/Inf_DBR6120.h"
#include "Inf/Inf_WTN6170.h"
#include "Inf/Inf_SC12B.h"
#include "Inf/Inf_WS2812.h"
#include "Driver/Dri_NVS.h"
#include "Com_Debug.h"

extern uint8_t isTouch;

uint8_t recData[20] = {0};
uint8_t recLen = 0;
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

    // 初始化WS2812 全彩led
    Inf_WS2812_Init();
    // 点亮所有led为白色
    Inf_WS2812_LightAllKeyLeds(white);

    // 初始化NVS模块
    Dri_NVS_Init();

    esp_err_t err = Dri_NVS_IsKeyExist((uint8_t *)"admin");
    if (err == ESP_OK)
    {
        MY_LOGE("admin key exist");
    }
    else
    {
        MY_LOGE("admin key not exist");
    }

    uint8_t data[3] = {'1', '2', '3'};
    Dri_NVS_WriteStr((uint8_t *)"aaa", data);

    err = Dri_NVS_ReadStr((uint8_t *)"aaa", recData, &recLen);
    MY_LOGE("err = %d", err);
    if (Dri_NVS_IsKeyExist((uint8_t *)"aaa") == ESP_OK)
    {
        MY_LOGE("recData = %s, recLen = %d", recData, recLen);
    }
    else
    {
        MY_LOGE("aaa key not exist");
    }

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
            if (touchKey != KEY_NO)
            {
                MY_LOGE("touchKey = %d", touchKey);
                // 点亮指定的按键led为红色
                Inf_WS2812_LightKeyLed(touchKey, red);
            }
            // 清除标记位
            isTouch = 0;
        }
        vTaskDelay(10);
    }
}
