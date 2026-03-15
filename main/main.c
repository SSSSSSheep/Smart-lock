#include <stdio.h>

#include "Inf_SC12B.h"
#include "esp_task.h"
#include "Inf_WS2812.h"
#include "App_IO.h"
#include "Inf_WTN6170.h"
#include "esp_log.h"
#include "App_Communication.h"
#include "Com_Debug.h"
#include "App_LowPower.h"

/* 1. 按键扫描任务 */
#define KEY_SCAN_TASK_NAME "key_scan_task"
#define KEY_SCAN_TASK_STACK (2 * 1024)
#define KEY_SCAN_TASK_PRIORITY 5
#define KEY_SCAN_TASK_EXCUTE_CYCEL 50
void keyScanTask(void *args);
TaskHandle_t keyScanTaskHandle;

/* 2. 指纹扫描任务 */
#define FINGERPRINT_SCAN_TASK_NAME "fingerprint_scan_task"
#define FINGERPRINT_SCAN_TASK_STACK (4 * 1024)
#define FINGERPRINT_SCAN_TASK_PRIORITY 5
#define FINGERPRINT_SCAN_TASK_EXCUTE_CYCEL 50
void fingerprintScanTask(void *args);
TaskHandle_t fingerprintScanTaskkHandle;

/* 3. 通讯任务 */
#define COMMUNICATION_TASK_NAME "communication_task"
#define COMMUNICATION_TASK_STACK (4 * 1024)
#define COMMUNICATION_TASK_PRIORITY 4
#define COMMUNICATION_TASK_EXCUTE_CYCEL 50
void communicationTask(void *args);
TaskHandle_t communicationHandle;

/* 4. 低功耗任务 */
#define LOW_POWER_TASK_NAME "low_power_task"
#define LOW_POWER_TASK_STACK (4 * 1024)
#define LOW_POWER_TASK_PRIORITY 3
#define LOW_POWER_TASK_EXCUTE_CYCEL 1000
void lowPowerTask(void *args);
TaskHandle_t lowPowerHandle;

#define VERSION "1.1.0"

int app_main(void)
{

    MY_LOGE("门锁项目版本: %s", VERSION);

    /* 1. 启动IO模块 */
    MY_LOGE("开始启动io模块");
    App_IO_Start();
    MY_LOGE("完成启动io模块");

    /* 2. 启动通讯模块 */
    MY_LOGE("开始启动通讯模块");
    App_Communication_Start();
    MY_LOGE("完成启动通讯模块");

    sayWaterDrop();

    /* 2.创建任务 */
    /* 2.1 创建键盘扫描任务 */
    xTaskCreate(
        keyScanTask,
        KEY_SCAN_TASK_NAME,
        KEY_SCAN_TASK_STACK,
        NULL,
        KEY_SCAN_TASK_PRIORITY,
        &keyScanTaskHandle);

    /* 2.2 创建指纹扫描任务 */
    xTaskCreate(
        fingerprintScanTask,
        FINGERPRINT_SCAN_TASK_NAME,
        FINGERPRINT_SCAN_TASK_STACK,
        NULL,
        FINGERPRINT_SCAN_TASK_PRIORITY,
        &fingerprintScanTaskkHandle);

    /* 2.3 通讯任务 */
    xTaskCreate(
        communicationTask,
        COMMUNICATION_TASK_NAME,
        COMMUNICATION_TASK_STACK,
        NULL,
        COMMUNICATION_TASK_PRIORITY,
        &communicationHandle);

    /* 2.4 低功耗任务 */
    xTaskCreate(
        lowPowerTask,
        LOW_POWER_TASK_NAME,
        LOW_POWER_TASK_STACK,
        NULL,
        LOW_POWER_TASK_PRIORITY,
        &lowPowerHandle);

    return 0;
}

/* 1. 按键扫描任务 */
void keyScanTask(void *args)
{
    MY_LOGE("按键扫描任务开始调度....");
    TickType_t preTime = xTaskGetTickCount();

    while (1)
    {
        /* 1. 启动按键扫描 */
        App_IO_KeyScan();

        vTaskDelayUntil(&preTime, KEY_SCAN_TASK_EXCUTE_CYCEL);
    }
}

/* 2. 指纹扫描任务 */
void fingerprintScanTask(void *args)
{
    MY_LOGE("指纹扫描任务开始调度...");
    /* 指纹模组初始化需要一段时间,最好等待模组初始化完成之后再进行指纹相关操作 */
    vTaskDelay(500);
    TickType_t preTime = xTaskGetTickCount();
    // Inf_FPM383_ClearAll();
    while (1)
    {
        App_IO_FingerPrintScan();
        vTaskDelayUntil(&preTime, FINGERPRINT_SCAN_TASK_EXCUTE_CYCEL);
    }
}

/* 3. 通讯任务 */
void communicationTask(void *args)
{
    MY_LOGE("通讯任务开始调度...");

    while (1)
    {
        uint32_t action = 0;
        /* 当按下按键3时开始ota */
        xTaskNotifyWait(0xffffffff, 0xffffffff, &action, portMAX_DELAY);
        if (action == '3')
        {
            App_Communication_OTA();
        }
    }
}

extern uint8_t isTouch;
extern uint8_t isHasFinger;
/* 4. 低功耗检测任务 */
void lowPowerTask(void *args)
{
    MY_LOGE("低功耗检测任务开始调度");
    TickType_t preTime = xTaskGetTickCount();
    uint8_t cnt = 0;

    while (1)
    {
        MY_LOGE("开始检测低功耗...");
        if (isTouch == 0 && isHasFinger == 0)
        {
            cnt++;

            if (cnt == 20) /* 连续10s没有手指触摸按键和指纹, 则进入低功耗 */
            {
                cnt = 0;
                App_LowPower_Enter();
                MY_LOGE("进入低功耗...");
            }
        }

        vTaskDelayUntil(&preTime, LOW_POWER_TASK_EXCUTE_CYCEL);
    }
}