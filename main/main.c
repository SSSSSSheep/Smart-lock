#include <stdio.h>
#include "esp_task_wdt.h"
#include "Inf/Inf_DBR6120.h"
#include "Inf/Inf_WTN6170.h"
#include "Inf/Inf_SC12B.h"
#include "Inf/Inf_WS2812.h"
#include "Inf/Inf_FPM383.h"
#include "Driver/Dri_NVS.h"
#include "Driver/Dri_WIFI.h"
#include "Com_Debug.h"
#include "Com_Config.h"
#include "App/App_IO.h"
#include "App/App_Communication.h"

void key_Scan_task(void *pvParameters);
TaskHandle_t keyScanHandle;

void finger_Scan_task(void *pvParameters);
TaskHandle_t fingerScanHandle;

void app_main(void)
{
    // wifi初始化
    // Dri_Wifi_Init();

    // // 测试指纹模块获取唯一设别号
    // Inf_FPM383_Init();
    // Inf_FPM383_ReadId();
    // Inf_FPM383_Sleep();

    // 1. 初始化所有基础模块
    App_IO_Init();
    App_Communication_Init();

    // 2. 创建一个读取按键的任务
    xTaskCreate(key_Scan_task, "key_Scan_task", 8192, NULL, 5, &keyScanHandle);
    // 3. 创建一个指纹模块的任务
    xTaskCreate(finger_Scan_task, "finger_Scan_task", 2048, NULL, 5, &fingerScanHandle);
}

void key_Scan_task(void *pvParameters)
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

/**
 * @brief 指纹扫描任务
 */
void finger_Scan_task(void *pvParameters)
{
    while (1)
    {

        // 等待通知，设置超时时间
        uint32_t notifyValue = 0;
        BaseType_t result = xTaskNotifyWait(UINT32_MAX, UINT32_MAX, &notifyValue, pdMS_TO_TICKS(5000));

        if (result == pdTRUE)
        {
            // 有通知，处理指纹操作
            App_IO_Finger();
        }

        vTaskDelay(200);
    }
}
extern uint8_t isTouch;
extern uint8_t hasFinger;