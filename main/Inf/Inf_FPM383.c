/* 指纹模块 */
#include "Inf_FPM383.h"

#define BUFF_SIZE 1024
#define RX_PIN GPIO_NUM_20
#define TX_PIN GPIO_NUM_21
#define TOUCH_OUT_PIN GPIO_NUM_10
#define FPM_UART_NUM UART_NUM_1

/*
命名包格式:
    包头(2)   设备地址(4)    包标识(1)   包长度(2)    指令(1)   参数(1-N)   校验和(2)
    EF 01    FF FF FF FF      01         xx xx        xx       xx *N      xx xx

注意:
    1.设备地址默认是 FFFFFF, 可以通过指令更改
    2.包长度至校验和（指令、参数或数据）的总字节数，包含校验和，
      但不包含包长度本身的字节数。
    3.校验和是从包标识至校验和之间所有字节之和，包含包标识不包含校验和，
      超出 2 字节的进位忽略
*/
uint8_t chipSNBuff[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x34, 0x00, 0x00, 0x39};
uint8_t sleepBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x33, 0x00, 0x37};
uint8_t emptyBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x0D, 0x00, 0x11};
uint8_t cancelBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x30, 0x00, 0x34};
uint8_t autoEnrollBuffer[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x31, '\0', '\0', 0x04, 0x00, 0x13, '\0', '\0'};   // PageID: bit 10:11，SUM: bit 15:16
uint8_t autoIdentifyBuffer[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x32, 0x03, 0xff, 0xff, 0x00, 0x03, '\0', '\0'}; // param: 13:14,sum: 15:16
uint8_t validTempleteNumBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x1d, 0x00, 0x21};
uint8_t indexTableBuffer[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x1f, '\0', '\0', '\0'};
uint8_t getImageBuffer[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x01, 0x00, 0x05};
uint8_t searchTempleteBuffer[17] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x08, 0x04, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x0C};
uint8_t deleteBuffer[16] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x07, 0x0C, '\0', '\0', 0x00, 0x01, '\0', '\0'}; // PageID: bit 10:11，SUM: bit 14:15

/* 接收指纹模块的缓冲区 */
uint8_t recvBuff[100];

void fpm383IntHandle(void *args);
/**
 * @description: 初始化
 *  我们是使用串口与指纹模块进行通讯的
 * @return {*}
 */
void Inf_FPM383_Init(void)
{
    /* 1. 串口参数配置 */
    uart_config_t uart_config = {
        .baud_rate = 57600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    /* 2. 安装驱动
        使用串口1(串口0被用于print了)
    */
    uart_driver_install(FPM_UART_NUM,
                        BUFF_SIZE,
                        0,
                        0,
                        NULL,
                        0);

    /* 3. 串口配置 */
    uart_param_config(FPM_UART_NUM, &uart_config);

    /* 4. 引脚设定 */
    uart_set_pin(FPM_UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    /* 5. 中断引脚配置 模组休眠后中断引脚低电平, 手指按压传感器后输出高电平脉冲 */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE; /* 上升沿触发 */
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pin_bit_mask = 1 << TOUCH_OUT_PIN;
    gpio_config(&io_conf);

    // gpio_install_isr_service(0);   // 添加一次即可
    /* 5.1 添加中断处理函数 */
    gpio_isr_handler_add(TOUCH_OUT_PIN, fpm383IntHandle, (void *)TOUCH_OUT_PIN);

    gpio_intr_disable(TOUCH_OUT_PIN);
    /* 6. 指纹模块进入休眠模式 */
    Inf_FPM383_Sleep();
}

volatile uint8_t isHasFinger = 0;

void fpm383IntHandle(void *args)
{
    esp_rom_printf("2");
    int pin = (int)args;
    if (pin == TOUCH_OUT_PIN)
    {
        isHasFinger = 1;
        gpio_intr_disable(TOUCH_OUT_PIN);
    }
}

/**
 * @description: 给指纹模组发送数据
 * @param {uint8_t} data
 * @param {uint16_t} dataLen
 * @return {*}  发送成功 OK , 其他情况返回ERROR
 */
Com_Status Inf_FPM383_SendData(uint8_t data[], uint16_t dataLen)
{

    int sendedBytes = uart_write_bytes(FPM_UART_NUM, data, dataLen);

    return sendedBytes == dataLen ? Com_OK : Com_ERROR;
}

/**
 * @description: 从模组读取数据, 接收到数据存储到 recvBuff缓冲区中
 * @param {uint16_t} dataLen 读取的数据的长度
 * @return {*} 读取成功 OK , 其他情况返回ERROR
 */
Com_Status Inf_FPM383_ReadData(uint16_t recvLen, uint32_t timeout)
{

    /* 清空缓冲区数组 */
    memset(recvBuff, 0, sizeof(recvBuff));
    int readedBytes = uart_read_bytes(FPM_UART_NUM, recvBuff, recvLen, timeout);
    uart_flush(FPM_UART_NUM);
    return readedBytes == recvLen ? Com_OK : Com_ERROR;
}

/**
 * @description: 读取指纹模块的id
 *  用来测试驱动, 模块是否正常
 * @return {*}
 */
void Inf_FPM383_ReadId(void)
{
    /* 1. 发送获取id的指令 */
    Inf_FPM383_SendData(chipSNBuff, sizeof(chipSNBuff));

    /* 2. 读取响应数据 根据手册: 共返回44个字节*/
    Com_Status status = Inf_FPM383_ReadData(44, 1000);
    if (status == Com_OK)
    {
        MY_LOGE("指纹模块id = %.32s", &recvBuff[10]);
    }
    else
    {
        MY_LOGE("指纹模块id获取失败");
    }
}

/**
 * @description: 呼吸灯控制
 * @param {uint8_t} fun
 *      1-普通呼吸灯，2-闪烁灯，3-常开灯，4-常闭灯，5-渐开灯，6-渐闭灯
 * @param {uint8_t} startColor
 *      设置为普通呼吸灯时，由灭到亮的颜色，只限于普通呼吸灯（功能码 01）功能，其他功能时，与结束颜色保持一致。
 *      其中，bit0 是蓝灯控制位；bit1 是绿灯控制位；bit2 是红灯控制位。
 *      置 1 灯亮，置 0 灯灭。
 *      例如 0x01_蓝灯亮，
 *           0x02_绿灯亮，
 *           0x04_红灯亮，
 *           0x06_红绿灯亮，
 *           0x05_红蓝灯亮，
 *           0x03_绿蓝灯亮，
 *           0x07_红绿蓝灯亮，
 *           0x00_全灭；
 * @param {uint8_t} endColor
 * @param {uint8_t} cycle
 *              表示呼吸或者闪烁灯的次数。
 *                  当设为 0 时，表示无限循环，
 *                  当设为其他值时，表示呼吸有限次数。
 *              循环次数适用于呼吸、闪烁功能，其他功能中无效，
 *                  例如在常开、常闭、开和渐闭中是无效的；
 * @return {*}
 */
void Inf_FPM383_LedControl(uint8_t fun, uint8_t startColor, uint8_t endColor, uint8_t cycle)
{
    uint8_t cmd[16] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 默认地址
        0x01, // 包标识
        0x00,
        0x07, // 包长度
        0x3C, // 指令码
        '\0', // 功能码占位符
        '\0', // 起始颜色占位符
        '\0', // 结束颜色占位符
        '\0', // 循环次数占位符
        '\0',
        '\0' // 校验和占位符
    };

    cmd[10] = fun;
    cmd[11] = startColor;
    cmd[12] = endColor;
    cmd[13] = cycle;

    Inf_FPM383_CheckSum(cmd, sizeof(cmd));

    Inf_FPM383_SendData(cmd, sizeof(cmd));
    Inf_FPM383_ReadData(12, portMAX_DELAY);
}

/**
 * @description: 取消指令: 取消自动注册模板和自动验证指纹。
 * @return {*}
 */
void Inf_FPM383_CanceAutoAction(void)
{
    do
    {
        Inf_FPM383_SendData(cancelBuffer, sizeof(cancelBuffer));
        Inf_FPM383_ReadData(12, portMAX_DELAY);
        printRecv("cancel", 12);
    } while (!(recvBuff[6] == 0x07 && recvBuff[9] == 0));
}

/**
 * @description: 注册用获取图像
 * @return {*}
 */
static Com_Status Inf_FPM383_GetImageWithRegister(void)
{

    uint8_t cmd[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x29, 0x00, 0x2d};
    Inf_FPM383_SendData(cmd, sizeof(cmd));
    Inf_FPM383_ReadData(12, portMAX_DELAY);
    printRecv("getImageWithRegister", 12);
    return recvBuff[9] == 0 ? Com_OK : Com_ERROR;
}

/**
 * @description: 生成特征
 * @return {*}
 */
static Com_Status Inf_FPM383_GenChar(void)
{
    uint8_t cmd[13] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x02, 0x01, 0x00, 0x08};
    Inf_FPM383_SendData(cmd, sizeof(cmd));
    Inf_FPM383_ReadData(12, portMAX_DELAY);
    printRecv("genchar", 12);
    return recvBuff[9] == 0 ? Com_OK : Com_ERROR;
}

/**
 * @description: 将特征文件融合后生成一个模板，结果存于模板缓冲区中。
 * @return {*}
 */
Com_Status Inf_FPM383_RegModel(void)
{
    uint8_t cmd[12] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x03, 0x05, 0x00, 0x09};
    Inf_FPM383_SendData(cmd, sizeof(cmd));
    Inf_FPM383_ReadData(12, portMAX_DELAY);
    printRecv("regmodel", 12);
    return recvBuff[9] == 0 ? Com_OK : Com_ERROR;
}

/**
 * @description: 储存模板
 * @param {uint16_t} pageId
 * @return {*}
 */
Com_Status Inf_FPM383_StoreChar(uint16_t pageId)
{
    uint8_t cmd[15] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x06, 0x06, 0x01, pageId >> 8, pageId, '\0', '\0'};

    Inf_FPM383_CheckSum(cmd, sizeof(cmd));

    Inf_FPM383_SendData(cmd, sizeof(cmd));
    Inf_FPM383_ReadData(12, portMAX_DELAY);
    printRecv("storechar", 12);

    return recvBuff[9] == 0 ? Com_OK : Com_ERROR;
}

/**
 * @description: 分布式注册指纹
 * @param {uint16_t} id 要注册的指纹id
 * @return {*}
 */
void Inf_FPM383_StepEnroll(uint16_t pageId)
{
    sayAddUserFingerprint(); /* 音效: 添加用户指纹 */

    /* 0. 搜索指纹如果已经存在, 则添加失败 */
    /* 0.1 扫描要删除的指纹 */
    if (Inf_FPM383_getImage() != Com_OK)
    {
        MY_LOGE("没有检测到手指...");
        sayOperateFail();
        return;
    }

    /* 0.2 查找该指纹 */
    int16_t id = Inf_FPM383_SearchTemplete();

    if (id != -1)
    {

        MY_LOGE("指纹已经注册过 id = %d", id);
        sayOperateFail();
        return;
    }

    uint8_t cnt = 0;
    while (cnt < 4)
    {
        vTaskDelay(3000);
        if (cnt == 0)
        {
            MY_LOGE("请放置手指");
            sayPlaceFinger();
        }
        else
        {
            MY_LOGE("请再次放置手指");
            sayPlaceFingerAgain();
        }
        /* 1. 采集手指图像 */
        if (Inf_FPM383_GetImageWithRegister() == Com_OK)
        {
            /* 2. 生成特征 */
            if (Inf_FPM383_GenChar() == Com_OK)
            {
                cnt++;
            }
        }
    }

    /* 3. 合并特征 */

    if (Inf_FPM383_RegModel() != Com_OK)
    {
        MY_LOGE("合并特征失败...");
        sayFingerprintAddFail();
        return;
    }

    /* 4. 保存模板 */
    if (Inf_FPM383_StoreChar(pageId) != Com_OK)
    {
        MY_LOGE("储存模板失败...");
        sayFingerprintAddSucc();
        return;
    }

    MY_LOGE("指纹注册成功");
    sayFingerprintAddSucc();
}

/**
 * @description: 自动指纹注册(一站式指纹注册)
 *                  包含采集指纹、生成特征、组合模板、存储模板等功能
 * @param {uint16_t} id  指纹id
 * @return {*}
 */
void Inf_FPM383_AutoEnroll(uint16_t id)
{

    sayAddUserFingerprint(); /* 音效: 添加用户指纹 */
    vTaskDelay(2000);
    autoEnrollBuffer[10] = id >> 8;
    autoEnrollBuffer[11] = id;

    /*
     参数= 0x13 = 0001 0011:
        0获取图像成功后led灭
        1打开预处理
        2在关键步骤返回状态
        3不允许覆盖其他id
        4不允许指纹重复注册
        5多次指纹采集过程中,手指需要离开
    */
    autoEnrollBuffer[14] = 0x13;

    uint16_t checkSum = 0;
    for (uint8_t i = 6; i < 15; i++)
    {
        checkSum += autoEnrollBuffer[i];
    }
    autoEnrollBuffer[15] = checkSum >> 8;
    autoEnrollBuffer[16] = checkSum;

    /* 估计是内部bug吧, 这里需要取消自动注册4次,后面的注册流程才能正常. */
    Inf_FPM383_CanceAutoAction();
    Inf_FPM383_CanceAutoAction();
    Inf_FPM383_CanceAutoAction();
    Inf_FPM383_CanceAutoAction();
    Inf_FPM383_SendData(autoEnrollBuffer, sizeof(autoEnrollBuffer));
    uint8_t confirmCode = 0,
            param1 = 0,
            param2 = 0;
    while (1)
    {
        /* 读取指令录入响应情况 */
        Inf_FPM383_ReadData(14, 10000);
        printRecv((char *)"enroll", 14);
        confirmCode = recvBuff[9]; /* 确认码 */
        param1 = recvBuff[10];     /* 参数1 */
        param2 = recvBuff[11];     /* 参数2 */

        uint8_t isFinish = 0;
        switch (confirmCode)
        {
        case 0x01:
        {
            MY_LOGE("指纹库已满,此次录入失败");

            sayFingerprintAddFail(); /* 音效: 指纹添加失败 */
            isFinish = 1;
            break;
        }
        case 0x22:
        {
            MY_LOGE("指纹id已经存在, 此次录入失败");
            sayFingerprintAddFail(); /* 音效: 指纹添加失败 */
            isFinish = 1;
            break;
        }
        case 0x27:
        {
            MY_LOGE("该手指指纹已经存在,此次录入失败");
            sayFingerprintAddFail(); /* 音效: 指纹添加失败 */
            isFinish = 1;
            break;
        }
        case 0x00:
        {
            if (param1 == 0x06) /* 参数1 */
            {
                MY_LOGE("指纹录入成功");
                sayFingerprintAddSucc(); /* 音效: 指纹添加成功 */
                isFinish = 1;
            }
            else if (param1 == 0x00 || param1 == 0x03)
            {
                static uint8_t fingerPressCnt = 0;
                fingerPressCnt++;
                if (fingerPressCnt == 1)
                {
                    MY_LOGE("请放入手指");
                    sayPlaceFinger(); /* 音效: 请放置手指 */
                }
                else if (fingerPressCnt <= 4)
                {
                    MY_LOGE("请拿开手指");
                    sayTakeAwayFinger(); /* 音效: 请拿开手指 */
                    vTaskDelay(2000);
                    MY_LOGE("请放入手指");
                    sayPlaceFinger(); /* 音效: 请放置手指 */

                    if (fingerPressCnt == 4)
                    {
                        fingerPressCnt = 0;
                    }
                }
            }
            break;
        }
        default:

            break;
        }

        if (isFinish)
            break;
    }
    Inf_FPM383_CanceAutoAction();

    MY_LOGE("此次指纹操作结束....");
}

/**
 * @description: 自动验证指纹
 *               自动采集指纹包含获取图像，生成特征，搜索指纹等功能。
 * @return {*}
 */
Com_Status Inf_FPM383_AutoIdentify(void)
{
    /* 1. 如果没有手指按下返回 OTHER */
    if (isHasFinger == 0)
        return Com_OTHER;

    isHasFinger = 0;

    Inf_FPM383_LedControl(3, 0x01, 0x01, 0); /* 点亮LED */

    vTaskDelay(300); /* 模组刚刚被唤醒, 等待300ms再去读取指纹验证 */

    /* 2. 如果有手指放入开始进行一站式验证 */
    Inf_FPM383_CheckSum(autoIdentifyBuffer, sizeof(autoIdentifyBuffer));

    Inf_FPM383_SendData(autoIdentifyBuffer, 17);

    uint8_t cnt = 5;
    while (cnt)
    {
        Inf_FPM383_ReadData(17, 2000);
        printRecv("idendtiy", 17);
        cnt--;

        if (recvBuff[9] != 0) /* 验证失败 */
        {
            return Com_ERROR;
        }
        else if (recvBuff[9] == 0 && recvBuff[10] == 5) /* 验证成功 */
        {
            /*如果确认码是 0(成功), 参数是:05(与已有指纹对比)*/
            return Com_OK;
        }
    }

    Inf_FPM383_CanceAutoAction();
    return Com_TIMEOUT;
}

/**
 * @description: 进入休眠指令
 * @return {*}
 */
void Inf_FPM383_Sleep(void)
{
    /* 一直等待休眠成功 */
    do
    {
        Inf_FPM383_SendData(sleepBuffer, sizeof(sleepBuffer));
        Inf_FPM383_ReadData(12, 1000);

        printRecv("sleep", 12);
    } while (recvBuff[9] != 0x00);

    Inf_FPM383_LedControl(4, 0x01, 0x01, 0); /* 关闭所有灯 */
    /* 开启引脚的中断功能 */
    gpio_intr_enable(TOUCH_OUT_PIN);
}

/**
 * @description: 清空所有指纹
 * @return {*}
 */
void Inf_FPM383_ClearAll(void)
{
    Inf_FPM383_SendData(emptyBuffer, sizeof(emptyBuffer));

    Inf_FPM383_ReadData(12, portMAX_DELAY);
    printRecv("clear all", 12);
}

/**
 * @description: 读取有效的模板数
 * @return {*}
 */
uint16_t Inf_FPM383_GetValidTempleteNum(void)
{
    Inf_FPM383_SendData(validTempleteNumBuffer, 12);

    Inf_FPM383_ReadData(14, portMAX_DELAY);

    return (recvBuff[10] << 10) + recvBuff[11];
}

/**
 * @description: 计算出一个最小的可用id
 * @return {*}
 */
uint8_t Inf_FPM383_GetMinAviableId(void)
{

    indexTableBuffer[10] = 0; /* 第0页的索引就可用了 */

    Inf_FPM383_CheckSum(indexTableBuffer, sizeof(indexTableBuffer));

    Inf_FPM383_SendData(indexTableBuffer, sizeof(indexTableBuffer));

    Inf_FPM383_ReadData(44, portMAX_DELAY);
    printRecv("indexTable", 44);

    /* 计算最小的可用id */
    for (uint8_t i = 0; i < 32; i++)
    {
        uint8_t v = recvBuff[i + 10];

        for (uint8_t j = 0; j < 8; j++)
        {
            if ((v & 0x01) == 0)
            {
                return i * 8 + j;
            }
            v >>= 1;
        }
    }

    return 0;
}

/**
 * @description: 验证指纹时，探测手指，探测到后录入指纹图像存于图像缓冲区。
 * @return {*} 获取图像是否成功
 */
Com_Status Inf_FPM383_getImage(void)
{
    uint8_t cnt = 5;
    do
    {
        MY_LOGE("请放入手指");
        sayPlaceFinger(); /* 语音: 请放入手指 */
        vTaskDelay(2000);

        Inf_FPM383_SendData(getImageBuffer, sizeof(getImageBuffer));
        Inf_FPM383_ReadData(12, portMAX_DELAY);
        printRecv("getImage", 12);
    } while (recvBuff[9] != 0 && --cnt);

    return recvBuff[9] == 0 ? Com_OK : Com_ERROR;
}

/**
 * @description: 搜索指纹
 * @return {*}  指纹id. -1 表示没有找到
 */
int16_t Inf_FPM383_SearchTemplete(void)
{
    Inf_FPM383_SendData(searchTempleteBuffer, sizeof(searchTempleteBuffer));
    Inf_FPM383_ReadData(16, portMAX_DELAY);

    printRecv("search", 16);
    return recvBuff[9] == 0 ? (recvBuff[10] << 8 | recvBuff[11]) : -1;
}

/**
 * @description: 删除指定的id
 * @param {uint16_t} pageId
 * @return {*}
 */
Com_Status Inf_FPM383_DeleteTemplete(uint16_t pageId)
{
    deleteBuffer[10] = pageId >> 8;
    deleteBuffer[11] = pageId;
    Inf_FPM383_CheckSum(deleteBuffer, sizeof(deleteBuffer));

    Inf_FPM383_SendData(deleteBuffer, sizeof(deleteBuffer));

    Inf_FPM383_ReadData(12, portMAX_DELAY);
    printRecv("delete", 12);
    return recvBuff[9] == 0 ? Com_OK : Com_ERROR;
}

/**
 * @description: 删除指纹
 * @return {*}
 */
void Inf_FPM383_DeleteFingerPrint(void)
{
    MY_LOGE("删除用户指纹...");
    sayDelUserFingerprint(); /* 删除用户指纹 */
    vTaskDelay(2000);
    /* 1. 扫描要删除的指纹 */
    if (Inf_FPM383_getImage() != Com_OK)
    {
        MY_LOGE("没有检测到手指...");
        sayOperateFail();
        return;
    }

    /* 2. 查找该指纹 */
    int16_t pageId = Inf_FPM383_SearchTemplete();

    if (pageId == -1)
    {
        MY_LOGE("要删除的指纹不存在");
        sayOperateFail();
        return;
    }

    /* 3. 删除该指纹 */
    Com_Status status = Inf_FPM383_DeleteTemplete(pageId);

    if (status != Com_OK)
    {
        MY_LOGE("删除失败 id = %d", pageId);
        sayDelFail(); /* 删除失败 */
    }

    MY_LOGE("删除成功 id = %d", pageId);
    sayDelSucc(); /* 删除成功 */
}

/**
 * @description: 计算校验和, 并把计算的结果放入到 buff[len - 2] 和 buff[len - 1]
 * @param {uint8_t} buff 要计算校验和的缓冲区
 * @param {uint8_t} buffLen 缓冲区的长度
 * @return {*}
 */
void Inf_FPM383_CheckSum(uint8_t buff[], uint8_t buffLen)
{
    uint16_t checkSum = 0;
    for (uint8_t i = 6; i < buffLen - 2; i++)
    {
        checkSum += buff[i];
    }
    buff[buffLen - 2] = checkSum >> 8;
    buff[buffLen - 1] = checkSum;
}

void printRecv(char *pre, uint8_t len)
{

    printf("%s = ", pre);
    for (uint8_t i = 0; i < len; i++)
    {
        printf("0x%02x ", recvBuff[i]);
    }
    printf("\n");
}
