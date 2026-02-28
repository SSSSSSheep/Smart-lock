#include <stdio.h>
#include "Inf/Inf_DBR6120.h"
#include "Inf/Inf_WTN6170.h"
#include "Inf/Inf_SC12B.h"
#include "Inf/Inf_WS2812.h"
#include "Driver/Dri_NVS.h"
#include "Inf/Inf_FPM383.h"
#include "Com_Debug.h"
#include "Com_Config.h"
#include "App/App_IO.h"

void Key_scan_task(void *pvParameters);
TaskHandle_t KeyScanHandle;

void app_main(void)
{
    // 测试指纹模块获取唯一设别号
    Inf_FPM383_Init();
    Inf_FPM383_ReadId();

    Inf_FPM383_Sleep();

    // 1. 初始化所有基础模块
    App_IO_Init();

    // 2. 创建一个读取按键的任务
    xTaskCreate(Key_scan_task, "Key_scan_task", 2048, NULL, 5, &KeyScanHandle);
}

void Key_scan_task(void *pvParameters)
{
    // 定义一个存储密码的数组
    uint8_t pwd[100] = {0};
    // 循环等待读取指令获取用户开锁密码
    while (1)
    {
        // 读取按键组合
        Com_Status comStatus = App_IO_ReadStr(pwd);

        // 根据返回值状态做不同处理
        switch (comStatus)
        {
        case Com_OK: // 以#号键结束
                     // 调用数据处理函数
            App_IO_Handler(pwd);
            break;
        case Com_ERROR: // 以M键结束
            sayIllegalOperation();
            break;
        case Com_TIMEOUT: // 超时
            Inf_WS2812_LightLedBlack();
            break;
        default: // 其他情况
            break;
        }

        // 清除密码数组
        memset(pwd, 0, sizeof(pwd));

        // 加一点延迟
        vTaskDelay(10);
    }
}

// void app_main(void)
// {
//     // 初始化电机模块
//     Inf_DBR6120_Init();

//     // Inf_DBR6120_Forward();
//     // vTaskDelay(1000);

//     // // 开锁
//     // Inf_DBR6120_OpenLock();

//     // // 初始化语音模块
//     // Inf_WTN6170_Init();

//     // // 发送指令
//     // Inf_WTN6170_SendCmd(0xF3);
//     // Inf_WTN6170_SendCmd(64);
//     // Inf_WTN6170_SendCmd(0xF3);
//     // Inf_WTN6170_SendCmd(65);

//     // 初始化SC12B按键模块

//     Inf_SC12B_Init();
//     Touch_Key touchKey = KEY_NO;

//     // 初始化WS2812 全彩led
//     Inf_WS2812_Init();
//     // 点亮所有led为白色
//     Inf_WS2812_LightAllKeyLeds(white);

//     // 初始化NVS模块
//     Dri_NVS_Init();

//     esp_err_t err = Dri_NVS_IsKeyExist((uint8_t *)"admin");
//     if (err == ESP_OK)
//     {
//         MY_LOGE("admin key exist");
//     }
//     else
//     {
//         MY_LOGE("admin key not exist");
//     }

//     uint8_t data[3] = {'1', '2', '3'};
//     Dri_NVS_WriteStr((uint8_t *)"aaa", data);

//     err = Dri_NVS_ReadStr((uint8_t *)"aaa", recData, &recLen);
//     MY_LOGE("err = %d", err);
//     if (Dri_NVS_IsKeyExist((uint8_t *)"aaa") == ESP_OK)
//     {
//         MY_LOGE("recData = %s, recLen = %d", recData, recLen);
//     }
//     else
//     {
//         MY_LOGE("aaa key not exist");
//     }

//     while (1)
//     {
//         // touchKey = Inf_SC12B_ReadKey();
//         // if (touchKey != KEY_NO)
//         // {
//         //     MY_LOGE("touchKey = %d", touchKey);
//         // }

//         if (isTouch)
//         {
//             touchKey = Inf_SC12B_ReadKey();
//             if (touchKey != KEY_NO)
//             {
//                 MY_LOGE("touchKey = %d", touchKey);
//                 // 点亮指定的按键led为红色
//                 Inf_WS2812_LightKeyLed(touchKey, red);
//             }
//             // 清除标记位
//             isTouch = 0;
//         }
//         vTaskDelay(10);
//     }
// }
