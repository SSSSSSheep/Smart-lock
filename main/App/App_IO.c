#include "App_IO.h"

uint8_t first_buffer[100] = {0};
uint8_t second_buffer[100] = {0};

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
static void App_IO_DelAdmin(void)
{
}
static void App_IO_AddUser(void)
{
}
static void App_IO_DelUser(void)
{
}
static void App_IO_CheckUser(void)
{
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
        // 删除普通用户
        else if (pwd[0] == '1' && pwd[1] == '1')
        {
            App_IO_DelUser();
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
        App_IO_CheckUser();
    }
}
