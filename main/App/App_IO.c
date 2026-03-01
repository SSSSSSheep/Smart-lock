#include "App_IO.h"

uint8_t first_buffer[100] = {0};
uint8_t second_buffer[100] = {0};

extern TaskHandle_t fingerScanHandle;
extern uint8_t hasFinger;

static void App_IO_ClearBuffer(void)
{
    memset(first_buffer, 0, sizeof(first_buffer));
    memset(second_buffer, 0, sizeof(second_buffer));
}

/**
 * @brief 添加管理员
 *  1.1 输入特殊指令(00#) 已经判断过
 *  1.2 按键输入管理员密码
 *  1.3 再次按键输入管理员密码确认
 *  1.4 确认密码一致后添加管理员
 *  1.5 根据比较结果做相应处理
 */
static void App_IO_AddAdmin(void)
{
    // 判断管理员密码是否存在
    if (Dri_NVS_IsKeyExist((uint8_t *)ADMIN_PWD) == ESP_OK) // 已存在
    {
        sayAdminFull();
    }
    else
    {
        sayWithoutInt();
        sayAddAdmin();
        sayWithoutInt();
        sayInputAdminPassword();

        // 按键输入管理员密码
        Com_Status comStatus = App_IO_ReadStr(first_buffer);
        // 根据当前获取按键组合返回值做不同处理
        switch (comStatus)
        {
        case Com_OK:
            // 再次输入管理员密码
            sayInputAdminPasswordAgain();
            Com_Status comStatus = App_IO_ReadStr(second_buffer);
            switch (comStatus)
            {
            case Com_OK:
                // 两次密码输入完成，比较密码是否一致
                if (strcmp((char *)first_buffer, (char *)second_buffer) == 0)
                {
                    // 密码一致，添加管理员写入FLASH
                    esp_err_t err = Dri_NVS_WriteStr((uint8_t *)ADMIN_PWD, first_buffer);
                    if (err == ESP_OK)
                    {
                        sayAddSucc();
                    }
                    else
                    {
                        sayAddFail();
                    }
                }
                else
                {
                    // 密码不一致
                    sayAddFail();
                }
                break;
            case Com_ERROR:
                sayIllegalOperation();
                break;
            default:
                break;
            }
            /* code */
            break;
        case Com_ERROR:
            sayIllegalOperation();
            break;
        default:
            break;
        }
        // 清理缓冲区
        App_IO_ClearBuffer();
    }
}

/**
 * @brief 检查管理员账号是否存在
 *
 */
static Com_Status App_IO_CheckAdmin(void)
{
    // 语音提示输入管理员账号
    sayWithoutInt();
    sayInputAdminPassword();

    // 读取按键组合
    Com_Status comStatus = App_IO_ReadStr(first_buffer);
    switch (comStatus)
    {
    case Com_OK:
        // 从FLASH中读取已经存在的管理员密码
        uint8_t len = 0;
        esp_err_t err = Dri_NVS_ReadStr((uint8_t *)ADMIN_PWD, second_buffer, &len);

        // 比较从键盘输入与FLASH中读取的密码长度是否一致
        if (strcmp((char *)first_buffer, (char *)second_buffer) == 0)
        {
            return Com_OK;
        }
        break;
    case Com_ERROR:
        sayIllegalOperation();
        break;
    default:
        break;
    }

    // 清理缓冲区
    App_IO_ClearBuffer();
    return Com_ERROR;
}

/*
    2.删除管理员账号
        1.1 输入特殊指令(01#) 输入已完成
        1.2 判断管理员账号是否存在
        1.3 按键输入管理员密码
        1.4 从FLASH中读取已经存在的管理员密码
        1.5 比较从键盘输入与FLASH中读取的密码
        1.6 根据比较结果做处理
 */
static void App_IO_DelAdmin(void)
{
    // 判断管理员账号是否存在
    if (Dri_NVS_IsKeyExist((uint8_t *)ADMIN_PWD) != ESP_OK) // 不存在
    {
        sayIllegalOperation();
        return;
    }
    else // 存在
    {
        sayWithoutInt();
        sayDelAdmin();
        Com_Status comStatus = App_IO_CheckAdmin();
        if (comStatus == Com_OK)
        {
            // 执行删除管理员操作
            esp_err_t err = Dri_NVS_DelKey((uint8_t *)ADMIN_PWD);
            if (err == ESP_OK)
            {
                sayDelSucc();
            }
            else
            {
                sayDelFail();
            }
        }
        else
        {
            sayDelFail();
        }
    }
}

/*
        3.注册普通用户账号
        1.1 输入特殊指令(10#)
        1.2 按键输入密码(管理员)
        1.3 从FLASH中读取已经存在的管理员密码
        1.4 比较从键盘输入与FLASH中读取的密码
        1.5 根据比较结果做处理
            比较结果不同:
                注册失败
            比较结果相同:
                按键输入密码(普通账号)
                再次输入密码(普通账号)
                比较两次输入结果
                根据比较结果做处理
*/
static void App_IO_AddUser(void)
{
    // 判断管理员是否存在
    if (Dri_NVS_IsKeyExist((uint8_t *)ADMIN_PWD) != ESP_OK) // 不存在
    {
        sayIllegalOperation();
    }
    else
    {
        // 验证管理员
        sayWithoutInt();
        sayAddUser();
        Com_Status comStatus = App_IO_CheckAdmin();

        if (comStatus == Com_OK)
        {
            // 按键输入密码
            sayInputUserPassword();
            comStatus = App_IO_ReadStr(first_buffer);
            switch (comStatus)
            {
            case Com_OK:
                // 再次输入密码
                sayInputUserPasswordAgain();
                comStatus = App_IO_ReadStr(second_buffer);

                switch (comStatus)
                {
                case Com_OK:
                    // 比较两次键盘读取的按键组合
                    if (strcmp((char *)first_buffer, (char *)second_buffer) == 0)
                    {
                        // 将用户密码写入FLASH
                        esp_err_t err = Dri_NVS_WriteStr((uint8_t *)first_buffer, (uint8_t *)"0");
                        if (err == ESP_OK)
                        {
                            // 密码一致
                            sayAddSucc();
                        }
                        else
                        {
                            // 密码不一致
                            sayAddFail();
                        }
                    }
                    else
                    {
                        // 密码不一致
                        sayAddFail();
                    }
                    break;
                case Com_ERROR:
                    sayIllegalOperation();
                    break;
                default:
                    break;
                }

                break;
            case Com_ERROR:
                sayIllegalOperation();
                break;
            default:
                break;
            }
        }
        else
        {
            sayAddFail();
        }
    }

    // 清理缓冲区
    App_IO_ClearBuffer();
}

/*
    4.删除普通用户账号
        1.1 输入特殊指令(11#)
        1.2 按键输入密码(管理员)
        1.3 从FLASH中读取已经存在的管理员密码
        1.4 比较从键盘输入与FLASH中读取的密码
        1.5 根据比较结果做处理
            比较结果不同:
                删除失败
            比较结果相同:
                按键输入密码(普通账号)
                从FLASH中读取已经存在的普通用户密码
                比较两次输入结果
                根据比较结果做处理
 */
static void App_IO_DelUser(void)
{
    // 判断管理员是否存在
    if (Dri_NVS_IsKeyExist((uint8_t *)ADMIN_PWD) != ESP_OK) // 不存在
    {
        sayIllegalOperation();
    }
    else
    {
        sayWithoutInt();
        sayDelUser();
        // 验证管理员
        Com_Status comStatus = App_IO_CheckAdmin();

        if (comStatus == Com_OK)
        {
            sayInputUserPassword();
            comStatus = App_IO_ReadStr(first_buffer);
            switch (comStatus)
            {
            case Com_OK:

                // 判断当前输入的数字组合在FLASH中是否存在
                if (Dri_NVS_IsKeyExist((uint8_t *)first_buffer) == ESP_OK)
                {
                    // 执行删除操作
                    esp_err_t err = Dri_NVS_DelKey((uint8_t *)first_buffer);
                    if (err == ESP_OK)
                    {
                        sayDelSucc();
                    }
                    else
                    {
                        sayDelFail();
                    }
                }
                else
                {
                    sayDelFail();
                }
                break;
            case Com_ERROR:
                sayIllegalOperation();
                break;
            default:
                break;
            }
        }
        else
        {
            sayDelFail();
        }
    }

    // 清理缓冲区
    App_IO_ClearBuffer();
}

/*
    5.验证普通用户密码开锁
        5.1 按键输入密码(普通账号)
        5.2 从FLASH中读取已经存在的普通用户密码
        5.3 比较两次输入结果
            比较结果不同:
                验证失败 -&gt; 请重试
            比较结果相同:
                验证成功 驱动电机开锁
 */
static void App_IO_CheckUser(uint8_t *pwd)
{
    // 判断当前输入的数字组合在FLASH中是否存在
    if (Dri_NVS_IsKeyExist(pwd) == ESP_OK)
    {
        sayVerifySucc();
        Inf_DBR6120_OpenLock();
        sayDoorOpen();
    }
    else
    {
        sayWithoutInt();
        sayVerifyFail();
        sayWithoutInt();
        sayRetry();
    }

    // 清理缓冲区
    App_IO_ClearBuffer();
}

void App_IO_Init(void)
{
    // 1. 电机初始化
    Inf_DBR6120_Init();
    // 2. 语音模块初始化
    Inf_WTN6170_Init();
    // 3. 按键模块初始化
    Inf_SC12B_Init();
    // 4. LED模块初始化
    Inf_WS2812_Init();
    Inf_WS2812_LightLedBlack();
    // 5. 初始化NVS模块
    Dri_NVS_Init();
    // 6. 指纹模块初始化
    Inf_FPM383_Init();
}

Com_Status App_IO_ReadStr(uint8_t *pwd)
{
    // 定义一个没有按键的时间变量
    uint8_t noKeyTime = 0;

    // 定义一个密码索引变量
    uint8_t pwdIndex = 0;

    while (1)
    {
        // 读取一次按键值
        Touch_Key touchKey = Inf_SC12B_GetKeyClick();

        // 判断是否有按键按下
        if (touchKey == KEY_NO)
        {
            // 没有按键按下，时间累加
            noKeyTime++;
            if (noKeyTime >= 100)
            {
                // 没有按键按下100次，认为输入完成
                return Com_TIMEOUT;
            }
        }
        else
        {
            // 有按键被按下,响声与LED灯
            Inf_WS2812_LightLedBlack();
            sayWaterDrop();
            Inf_WS2812_LightKeyLed(touchKey, white);

            // 有按键按下，时间重置
            noKeyTime = 0;

            // 根据具体按键做不同处理
            if (touchKey == KEY_M)
            {
                return Com_ERROR;
            }
            else if (touchKey == KEY_SHARP)
            {
                return Com_OK;
            }
            else // 数字键
            {
                // 记录下数字键
                pwd[pwdIndex++] = touchKey + 48; // 将按键值转换为对应的ASCII码
            }
        }

        vTaskDelay(50);
    }
}

/*
   按键组合
   00#：注册管理员
   01#：删除管理员
   10#：注册用户
   11#：删除用户
   99+：删除所有用户
 */
void App_IO_Handler(uint8_t *pwd)
{
    // 取出数组中元素个数
    uint8_t len = strlen((char *)pwd);

    // 根据不同的长度做不同的处理
    if (len < 2)
    {
        sayIllegalOperation();
    }
    else if (len == 2)
    {
        // 注册管理员
        if (pwd[0] == '0' && pwd[1] == '0')
        {
            App_IO_AddAdmin();
        }
        // 删除管理员
        else if (pwd[0] == '0' && pwd[1] == '1')
        {
            App_IO_DelAdmin();
        }
        // 添加普通用户
        else if (pwd[0] == '1' && pwd[1] == '0')
        {
            App_IO_AddUser();
        }
        // 注册指纹
        else if (pwd[0] == '2' && pwd[1] == '0')
        {
            // 验证管理员
            sayWithoutInt();
            sayAddUserFingerprint();
            Com_Status comStatus = App_IO_CheckAdmin();
            // 通知指纹模块任务
            if (comStatus == Com_OK)
            {
                xTaskNotify(fingerScanHandle, (uint32_t)1, eSetValueWithOverwrite);
            }
            else
            {
                sayVerifyFail();
            }
        }
        // 删除指纹
        else if (pwd[0] == '2' && pwd[1] == '1')
        {
            // 验证管理员
            sayWithoutInt();
            sayDelUserFingerprint();
            Com_Status comStatus = App_IO_CheckAdmin();
            // 通知指纹模块任务
            if (comStatus == Com_OK)
            {
                xTaskNotify(fingerScanHandle, (uint32_t)2, eSetValueWithOverwrite);
            }
            else
            {
                sayVerifyFail();
            }
        }
        // 删除所有指纹
        else if (pwd[0] == '8' && pwd[1] == '8')
        {
            sayDelAll();
            Inf_FPM383_DelAllFingerPrint();
        }
        // 删除所有用户
        else if (pwd[0] == '9' && pwd[1] == '9')
        {
            sayDelAll();
            Dri_NVS_DelAll();
        }
        // 指令不存在
        else
        {
            sayIllegalOperation();
        }
    }
    // 第一次输入超过两个数字，表示在验证密码开锁
    else
    {
        App_IO_CheckUser(pwd);
    }
}

void App_IO_Finger(void)
{
    // 等待通知
    uint32_t notifyValue = 0;
    xTaskNotifyWait(UINT32_MAX, UINT32_MAX, &notifyValue, 0);

    // 根据操作值进行不同的业务处理
    if (notifyValue)
    {
        // 关闭指纹模块的中断 防止在注册或者删除之后自动走一次验证代码
        gpio_intr_disable(Inf_FPM383_INTR_PIN);

        // 注册指纹逻辑
        if (notifyValue == 1)
        {
            sayPlaceFinger();
            vTaskDelay(1500);
            // 获取最小的可用ID
            uint16_t id = Inf_FPM383_GetMinId();
            MY_LOGE("minId:%d", id);

            // 注册指纹
            Com_Status comStatus = Inf_FPM383_AddFingerPrint(id);

            if (comStatus == Com_OK)
            {
                sayFingerprintAddSucc();
            }
            else
            {
                sayFingerprintAddFail();
            }
        }
        // 删除指纹逻辑
        else if (notifyValue == 2)
        {
            sayPlaceFinger();
            vTaskDelay(1500); // 听完语音以后再放手指
            // 获取当前放置手指的ID
            uint16_t id = Inf_FPM383_FindFingerPrint();
            MY_LOGE("DEL ID = %d", id);

            if (id != -1)
            {
                // 删除指纹
                Com_Status comStatus = Inf_FPM383_DelFingerPrint(id);
                if (comStatus == Com_OK)
                {
                    sayDelSucc();
                }
                else
                {
                    sayDelFail();
                }
            }
            else
            {
                sayDelFail();
            }
        }

        // 进入休眠模式 由于芯片本身有自己的定时器 所以不需要手动进入休眠模式
        // Inf_FPM383_Sleep();
        vTaskDelay(2000);
        esp_restart();
    }
    // 默认清空 没有任务通知 表示验证指纹
    else
    {
        // 判断是否有手指按下
        if (hasFinger)
        {
            // 清理标记位
            hasFinger = 0;

            // 验证指纹 开锁
            Com_Status comStatus = Inf_FPM383_CheckFingerPrint();
            if (comStatus == Com_OK)
            {
                sayVerifySucc();
                Inf_DBR6120_OpenLock();
                sayDoorOpen();
            }
            else
            {
                sayWithoutInt();
                sayVerifyFail();
                sayWithoutInt();
                sayRetry();
            }

            // 进入休眠模式 由于芯片本身有自己的定时器 所以不需要手动进入休眠模式
            // Inf_FPM383_Sleep();
            esp_restart();
        }
    }
}
