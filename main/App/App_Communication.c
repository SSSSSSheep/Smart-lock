#include "App_Communication.h"

/**
 * @brief 添加密码
 * @param pwd 密码指针
 */
static void App_Communication_AddPwd(uint8_t *pwd)
{
    Com_Status comStatus = Dri_NVS_WriteStr(pwd, (uint8_t *)"0");
    if (comStatus == ESP_OK)
    {
        sayAddSucc();
    }
    else
    {
        sayAddFail();
    }
}

/**
 * @brief 删除密码
 * @param pwd 密码指针
 */
static void App_Communication_DelPwd(uint8_t *pwd)
{
    if (Dri_NVS_IsKeyExist(pwd) != ESP_OK)
    {
        sayDelFail();
    }
    else
    {
        esp_err_t err = Dri_NVS_DelKey(pwd);
        if (err == ESP_OK)
        {
            sayDelSucc();
        }
        else
        {
            sayDelFail();
        }
    }
}

/**
 * @brief 验证密码开锁
 * @param pwd 密码指针
 */
static void App_Communication_CheckPwd(uint8_t *pwd)
{
    if (Dri_NVS_IsKeyExist(pwd) != ESP_OK)
    {
        sayVerifyFail();
    }
    else
    {
        sayVerifySucc();
        Inf_DBR6120_OpenLock();
        sayDoorOpen();
    }
}

void App_Communication_Init(void)
{
    Dri_BLE_Init();
}

/**
 * @brief 处理esp32收到手机数据时的回调函数
 * @param data 收到的数据指针 设备号+操作类型+密码 xxx+0/添加1/删除2/验证+666666
 * @param dataLen 收到的数据长度
 * 添加密码
 * 删除密码
 * 验证密码开锁
 */
void App_Communication_RecvDataCb(uint8_t *data, uint16_t dataLen)
{
    printf("Value = %s,Len = %d\r\n", data, dataLen);

    // 获取手机端传输过来的数据中的密码
    uint8_t pwd[100] = {0};
    memcpy(pwd, &data[2], dataLen - 2);
    switch (data[0])
    {
    case '0': // 添加密码
        App_Communication_AddPwd(pwd);
        break;
    case '1': // 删除密码
        App_Communication_DelPwd(pwd);
        break;
    case '2': // 验证密码开锁
        App_Communication_CheckPwd(pwd);
        break;
    }
}
