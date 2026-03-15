/*
IO模块:
    1. 按键输入
    2. led显示
    3. 语音输出
    4. 电机驱动
    5. 指纹输入

键盘输入协议:
    1. 所有输入都是以 # 结束
    2. 输入M位非法输入, 以前所有输入作废
    3. 协议规则
            01#  新增密码
            02#  删除密码
                ...


            10#  新增指纹
            11#  删除指纹
                ...

            20#  OTA更新
                ...
    4. 数字超过2位的认为时在输入密码开门
 */
#include "App_IO.h"

/* 临时存储用户输入的密码 (把数字转成它的字符形式存储, 方便后面当做字符串处理) */
static uint8_t password[100] = {0};
/* 临时存储的用户输入的密码的字符数 */
static uint8_t passwordLen = 0;

static void App_IO_InputHandle(void);

/**
 * @description: 输入模块启动
 * @return {*}
 */
void App_IO_Start(void)
{
    /* 1. 触摸感应器初始化 */
    Inf_SC12B_Init();
    /* 2. LED初始化 */
    Inf_WS2812_Init();
    /* 3. nvs初始化 */
    Dri_NVS_Init();
    /* 4. 初始化语音输出模块 */
    Inf_WTN6170_Init();
    /* 5. 电机驱动芯片初始化 */
    Inf_BDR6120_Init();
    /* 6. 指纹初始化 */
    Inf_FPM383_Init();
}

/**
 * @description: 按键扫描
 *
 *    密码输入和设定 状态机:  共分为3个状态
 *      0:自由状态:       默认状态. 在此状态下, 如果检测到有任何按键, 则进入 1:密码输入阶段
 *
 *      1:密码输入阶段
 *                      保存密码
 *      2:输入完成阶段
 *                      对输入密码根据协议进行各种处理
 *
 * @return {*}
 */
/* 输入状态 */
static Input_Status inputStatus = FREE;

/* 修改 App_IO.c 中的 App_IO_KeyScan 函数 */
void App_IO_KeyScan(void)
{
    /* 没有输入按键的时间 */
    static uint16_t noKeyTime = 0;

    Touch_Key key = Inf_SC12B_KeyClick();
    /* 1. 如果没有按键按下直接返回 */
    if (key == KEY_NO)
    {
        noKeyTime++;
        if (noKeyTime >= 5 * 20) /* 如果超过5s没有输入密码, 则关闭所有led */
        {
            Inf_WS2812_LightAllKeyLeds(black);
            noKeyTime = 5 * 20; /* 防止nokeyTime溢出 */
            inputStatus = FREE; /* 输入状态进入自由状态 */
        }
        return;
    }

    noKeyTime = 0;
    /* 2. 状态处理 */
    switch (inputStatus)
    {
    /* 自由阶段: 有按键按下, 相当于激活键盘, 此次按键不报错, 然后进入输入阶段 */
    case FREE:
    {
        Inf_WS2812_LightAllKeyLeds(white);
        inputStatus = INPUT;

        // 直接处理当前按键，避免下次循环再次处理
        if (key == KEY_M)
        {
            MY_LOGE("输入了M, 非法输入");
            sayIllegalOperation();
            inputStatus = FREE;
        }
        else if (key == KEY_SHARP)
        {
            inputStatus = DONE;
            /* 水滴声 */
            sayWaterDrop();
            vTaskDelay(100);

            /* 对输入进行处理 */
            App_IO_InputHandle();

            /*
                输入处理之后:
                    1. 状态进入自由状态
                    2. 密码清零
            */
            inputStatus = FREE;
            passwordLen = 0;
            memset((char *)password, 0, sizeof(password));
        }
        else
        {
            /*不是 # 就保存密码  存储他们的字符形式  0->'0  1->'1' */
            password[passwordLen++] = key + 48;
            /* 水滴声 */
            sayWaterDrop();
        }
        break;
    }
    /* 输入阶段: 存储输入的按键, 存储字符而不是数字 对0-9来说, +48变成对应的字符  */
    case INPUT:
    {
        /* 灯光处理 */
        Inf_WS2812_LightAllKeyLeds(black);
        vTaskDelay(30);
        Inf_WS2812_LightKeyLed(key, blue);

        if (key == KEY_M)
        {
            MY_LOGE("输入了M, 非法输入");
            sayIllegalOperation();
            inputStatus = FREE;
        }
        else if (key == KEY_SHARP)
        {
            inputStatus = DONE;
            /* 水滴声 */
            sayWaterDrop();
            vTaskDelay(100);

            /* 对输入进行处理 */
            App_IO_InputHandle();

            /*
                输入处理之后:
                    1. 状态进入自由状态
                    2. 密码清零
            */
            inputStatus = FREE;
            passwordLen = 0;
            memset((char *)password, 0, sizeof(password));
        }
        else
        {
            /*不是 # 就保存密码  存储他们的字符形式  0->'0  1->'1' */
            password[passwordLen++] = key + 48;
            /* 水滴声 */
            sayWaterDrop();
        }

        break;
    }
    default:
        break;
    }
}

static uint8_t isAddPwd = 0;
static uint8_t isDelPwd = 0;
/**
 * @description: 当输入结束之后, 开始对输入进行处理
 * @return {*}
 */
static void App_IO_InputHandle(void)
{

    MY_LOGE("开始处理输入, 输入: %s", password);
    if (passwordLen < 2)
    {
        MY_LOGE("非法输入");
        sayIllegalOperation();
    }
    else if (passwordLen == 2)
    {
        /* 命令输入 */
        if (password[0] == '0' && password[1] == '1') /*添加密码 */
        {
            isAddPwd = 1;

            MY_LOGE("添加密码");
            sayAddUser();
            vTaskDelay(1000);
            sayPassword();
            vTaskDelay(200);
        }
        else if (password[0] == '0' && password[1] == '2') /* 删除密码 */
        {
            isDelPwd = 1;

            MY_LOGE("删除密码");
            sayDelUser();
            vTaskDelay(500);
            sayPassword();
            vTaskDelay(200);
        }
        else if (password[0] == '1' && password[1] == '1') /* 添加指纹 */
        {
            xTaskNotify(fingerprintScanTaskkHandle, (uint32_t)'1', eSetValueWithOverwrite);
        }
        else if (password[0] == '1' && password[1] == '2') /* 删除指纹 */
        {
            xTaskNotify(fingerprintScanTaskkHandle, (uint32_t)'2', eSetValueWithOverwrite);
        }
        else if (password[0] == '2' && password[1] == '1') /* ota */
        {
            xTaskNotify(communicationHandle, (uint32_t)1, eSetValueWithOverwrite);
        }
    }
    else if (passwordLen > 2)
    {
        if (isAddPwd)
        {
            isAddPwd = 0;
            App_IO_AddPwd(password, passwordLen);
        }
        else if (isDelPwd)
        {
            isDelPwd = 0;
            App_IO_DelPwd(password);
        }
        else
        {
            App_IO_CheckPwd(password);
        }
    }
}

/**
 * @description: 添加密码
 *   我们使用密码直接作为key, value存储个u8 0即可
 *      好处:
 *          1.方便存取
 *          2.自动去重
 *          3.减少空间占用
 *
 * @return {*}
 */
void App_IO_AddPwd(uint8_t *pwd, uint8_t pwdLen)
{

    /* 1. 设置的密码长度 判断  不小于5*/
    if (pwdLen < 5)
    {
        MY_LOGE("密码长度: %d < 5, 新的密码存储失败!", pwdLen);
        sayPasswordAddFail();
        return;
    }

    /* 2. 验证密码个数不能超过100 */
    uint8_t pwdCnt = 0;
    Dri_NVS_ReadU8("pwd_cnt", &pwdCnt);
    if (pwdCnt >= 100)
    {
        MY_LOGE("密码已满 100 个, 新的密码存储失败!");
        sayPasswordAddFail();
        return;
    }
    /* 3. 存储密码 */
    /* 3.1. 存储密码个数  密码个数+1*/
    pwdCnt++;
    Dri_NVS_WriteU8("pwd_cnt", pwdCnt);
    /* 3.2. 存储密码 */
    esp_err_t err = Dri_NVS_WriteU8((char *)pwd, 0);
    if (err == ESP_OK)
    {
        MY_LOGE("密码存储成功");
        sayPasswordAddSucc();
    }
    else
    {
        MY_LOGE("密码存储失败");
        sayPasswordAddFail();
    }
}

/**
 * @description: 当用密码开门时, 读取键盘输入的密码, 然后与NVS中存储的密码做比对
 * @return {*}
 */
void App_IO_CheckPwd(uint8_t *pwd)
{
    if (Dri_NVS_IsKeyExist(pwd, 0) == Com_OK)
    {
        MY_LOGE("密码验证成功");
        sayPasswordVerifySucc();
        Inf_BDR6120_LockOpen(); /* 驱动电机开锁 */
        sayDoorOpen();
        MY_LOGE("门已开");
    }
    else
    {
        MY_LOGE("密码错误, 请重试...");
        sayPasswordVerifyFail(); /* 验证失败 */
        vTaskDelay(1500);
        sayRetry();
    }
}

/**
 * @description: 删除密码
 * @return {*}
 */
void App_IO_DelPwd(uint8_t *pwd)
{
    esp_err_t err = nvs_erase_key(my_nvs_handle, (char *)pwd);
    if (err == ESP_OK)
    {
        MY_LOGE("删除成功");
        sayDelSucc();
    }
    else
    {
        MY_LOGE("删除失败");
        sayDelFail();
    }
}

/**
 * @description: 手指扫描
 * @return {*}
 */
void App_IO_FingerPrintScan(void)
{
    uint32_t action = 0;
    xTaskNotifyWait(0xffffffff, 0xffffffff, &action, 0);

    if (action != 0) /* 有指纹相关操作: 添加指纹  删除指纹 */
    {
        if (action == '1') /* 添加指纹 */
        {
            LED_GREEN_FLICKER;
            uint8_t id = Inf_FPM383_GetMinAviableId();
            MY_LOGE("id = %d", id);
            Inf_FPM383_AutoEnroll(id);
            // Inf_FPM383_StepEnroll(id);
            Inf_FPM383_Sleep();
            esp_restart();
        }
        else if (action == '2') /* 删除指纹 */
        {
            LED_RED_FLICKER;
            Inf_FPM383_DeleteFingerPrint();
            Inf_FPM383_Sleep();
            esp_restart();
        }
    }
    else
    {
        Com_Status isOk = Inf_FPM383_AutoIdentify();
        if (isOk == Com_OK)
        {
            MY_LOGE("指纹验证通过...");
            sayFingerprintVerifySucc(); /* 语音: 指纹验证成功 */
            Inf_BDR6120_LockOpen();
            sayDoorOpen(); /* 开门: 音效 */
            Inf_FPM383_Sleep();
        }
        else if (isOk == Com_ERROR)
        {
            MY_LOGE("指纹验证失败...");
            sayFingerprintVerifyFail(); /* 语音: 指纹验证失败 */
            Inf_FPM383_Sleep();
        }
        else if (isOk == Com_TIMEOUT)
        {
            Inf_FPM383_Sleep();
        }
    }
}

void App_IO_PrintfPassword(void)
{
    MY_LOGE("密码长度: %d", passwordLen);
    printf("密码内容:");
    for (uint8_t i = 0; i < passwordLen; i++)
    {
        printf("%c", password[i]);
    }
    printf("\r\n");
}
