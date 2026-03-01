#include "Inf_FPM383.h"

// 接收缓冲区大小
static const int RX_BUF_SIZE = 1024;

static uint8_t rx_buf[100] = {1};

uint8_t hasFinger = 0;
static void Inf_FPM383_Intr_Handler(void *args)
{
    // esp_rom_printf("Inf_FPM383_Intr_Handler\n");
    hasFinger = 1;
}

/**
 * @brief 通过UART发送数据到FPM383模块
 *
 * @return Com_Status
 */
static Com_Status Inf_FPM383_SendData(uint8_t *data, uint8_t len)
{
    int sendLen = uart_write_bytes(UART_NUM_1, data, len);
    if (sendLen == len)
    {
        return Com_OK;
    }
    else
    {
        return Com_ERROR;
    }
}

/**
 * @brief 通过UART接收FPM383模块返回的数据
 *
 * @return Com_Status
 */
static Com_Status Inf_FPM383_RecvData(uint8_t len, uint16_t time_out)
{
    // 清空缓存区
    memset(rx_buf, 0, sizeof(rx_buf));
    int recvLen = uart_read_bytes(UART_NUM_1, rx_buf, len, time_out);
    if (recvLen == len)
    {
        return Com_OK;
    }
    else
    {
        return Com_ERROR;
    }
}

/**
 * @brief 计算指令的校验和
 * @param cmd 指令数据
 * @param len 指令长度
 */
static void Inf_FPM383_AddChecksum(uint8_t *cmd, uint8_t len)
{
    // 定义一个校验和结果
    uint16_t checkSum = 0;

    // 循环添加对应的字节
    for (uint8_t i = 6; i < len - 2; i++)
    {
        checkSum += cmd[i];
    }

    // 补充校验和至指令集
    cmd[len - 2] = (checkSum >> 8);
    cmd[len - 1] = checkSum;
}

void Inf_FPM383_Init(void)
{
    // 1. UART模块
    // 1.1 UART配置信息
    const uart_config_t uart_config = {
        .baud_rate = 57600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // 1.2 安装UART驱动
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);

    // 1.3 使配置信息生效
    uart_param_config(UART_NUM_1, &uart_config);

    // 1.4 绑定引脚
    uart_set_pin(UART_NUM_1, Inf_FPM383_TX_PIN, Inf_FPM383_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // 2. 中断
    // 2.1 中断引脚配置
    gpio_config_t io_config = {
        .pin_bit_mask = (1 << Inf_FPM383_INTR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io_config);

    // 2.2 安装中断服务，采用默认参数
    gpio_install_isr_service(0);

    // 2.3 注册回调函数
    gpio_isr_handler_add(Inf_FPM383_INTR_PIN, Inf_FPM383_Intr_Handler, (void *)Inf_FPM383_INTR_PIN);

    // 2.4 关闭中断
    gpio_intr_disable(Inf_FPM383_INTR_PIN);

    // 3. 进入休眠模式 开启中断
    Inf_FPM383_Sleep();
}

void Inf_FPM383_Sleep(void)
{
    // 1. 准备指令集
    uint8_t cmd[12] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x03, // 包长度
        0x33, // 指令码
        0x00,
        0x37, // 校验和

    };

    // 2.发送指令进入休眠模式 直至成功
    do
    {
        MY_LOGE("即将进入休眠模式...");
        // 2.1 发送指令
        Inf_FPM383_SendData(cmd, 12);

        // 2.2 接收应答包
        Inf_FPM383_RecvData(12, 2000);
    } while (rx_buf[9] != 0x00);

    // 3. 休眠成功 开启中断 发送任意指令可以唤醒模块
    MY_LOGE("进入休眠模式成功...");
    gpio_intr_enable(Inf_FPM383_INTR_PIN);
}

void Inf_FPM383_ReadId(void)
{
    // 1. 准备指令集
    uint8_t cmd[13] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x04, // 包长度
        0x34, // 指令码
        0x00, // 参数
        0x00,
        0x39, // 校验和
    };

    // 2. 发送指令集
    Inf_FPM383_SendData(cmd, 13);

    // 3. 接收应答包
    Inf_FPM383_RecvData(44, 3000);

    // 4. 判断是否读取到设备号
    if (rx_buf[9] == 0x00)
    {
        // 4.1 读取到设备号
        MY_LOGE("读取到设备号: %.32s", rx_buf + 10);
    }
    else
    {
        MY_LOGE("读取设备号失败");
    }
}

uint16_t Inf_FPM383_GetMinId(void)
{
    // 1. 准备指令集
    uint8_t cmd[13] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x04, // 包长度
        0x1f, // 指令码
        0x00, // 页码
        '\0',
        '\0', // 校验和
    };

    // 2. 补充校验和
    Inf_FPM383_AddChecksum(cmd, 13);

    // 3. 发送指令
    Inf_FPM383_SendData(cmd, 13);

    // 4. 获取应答包
    Inf_FPM383_RecvData(44, 3000);

    // 5. 一次遍历索引，找到最小的为0的位置
    for (uint8_t i = 0; i < 32; i++)
    {
        uint8_t byte = rx_buf[10 + i];
        // 对单个字节进行从低位遍历
        for (uint8_t j = 0; j < 8; j++)
        {
            if (byte & 0x01)
            {
                byte >>= 1;
            }
            else
            {
                return i * 8 + j;
            }
        }
    }

    // 如果遍历完所有字节都没有找到为0的位置，则返回0
    return 0;
}

void Inf_FPM383_CancelAutoAction(void)
{
    // 1. 准备指令集
    uint8_t cmd[12] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x04, // 包长度
        0x30, // 指令码
        '\0',
        '\0', // 校验和
    };

    // 2. 补充校验和
    Inf_FPM383_AddChecksum(cmd, 12);

    do
    {
        // 3. 发送指令
        Inf_FPM383_SendData(cmd, 12);

        // 4. 获取应答包
        Inf_FPM383_RecvData(12, 2000);
    } while (rx_buf[9] != 0x00);

    // 5. 取消自动操作成功
    MY_LOGE("取消自动操作成功");
}

Com_Status Inf_FPM383_AddFingerPrint(uint16_t id)
{
    // 1. 准备指令集
    uint8_t cmd[17] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x08, // 包长度
        0x31, // 指令码
        '\0',
        '\0', // ID号
        0x02, // 录入次数
        0x00,
        0x32, // 参数
        '\0',
        '\0', // 校验和
    };

    // 2. 补充校验和 及 ID号
    cmd[10] = (id >> 8);
    cmd[11] = id;
    Inf_FPM383_AddChecksum(cmd, 17);

    // 3. 取消自动操作
    Inf_FPM383_CancelAutoAction();
    Inf_FPM383_CancelAutoAction();
    Inf_FPM383_CancelAutoAction();
    Inf_FPM383_CancelAutoAction();

    // 4. 发送指令集
    Inf_FPM383_SendData(cmd, 17);

    // 5. 获取应答包，并判断是录入成功还是失败，只要中间任何一次返回确认码不是0x00,均为失败
    while (1)
    {
        // 依次接收不同的应答包
        Inf_FPM383_RecvData(14, 2000);

        if (rx_buf[9] != 0x00)
        {
            return Com_ERROR;
        }
        else if (rx_buf[10] == 0x06)
        {
            return Com_OK;
        }
    }
}

Com_Status Inf_FPM383_CheckFingerPrint(void)
{
    // 1. 准备指令集
    uint8_t cmd[17] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x08, // 包长度
        0x32, // 指令码
        0x03, // 分数等级
        0xff,
        0xff, // ID号，全1表示与指纹库所有存入的指纹进行比较
        0x00,
        0x06, // 参数
        '\0',
        '\0', // 校验和
    };

    // 2. 补充校验和
    Inf_FPM383_AddChecksum(cmd, 17);

    // 3. 发送指令集
    Inf_FPM383_SendData(cmd, 17);

    // 4. 接收应答包 由于参数设置为不反回中间状态的应答包 所以只需要接收一次
    Inf_FPM383_RecvData(17, 2000);

    // 5. 解析应答包
    if (rx_buf[9] == 0x00) // 成功
    {
        MY_LOGE("Check ID = %d", rx_buf[12]);
        return Com_OK;
    }
    else
    {
        return Com_ERROR;
    }
}

int Inf_FPM383_FindFingerPrint(void)
{
    // 1. 准备指令集
    uint8_t cmd[17] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x08, // 包长度
        0x32, // 指令码
        0x03, // 分数等级
        0xff,
        0xff, // ID号，全1表示与指纹库所有存入的指纹进行比较
        0x00,
        0x06, // 参数
        '\0',
        '\0', // 校验和
    };

    // 2. 补充校验和
    Inf_FPM383_AddChecksum(cmd, 17);

    // 3. 发送指令集
    Inf_FPM383_SendData(cmd, 17);

    // 4. 接收应答包 由于参数设置为不反回中间状态的应答包 所以只需要接收一次
    Inf_FPM383_RecvData(17, 2000);

    // 5. 解析应答包
    if (rx_buf[9] == 0x00) // 成功
    {
        MY_LOGE("Find ID = %d", rx_buf[12]);
        return rx_buf[12];
    }
    else
    {
        return -1;
    }
}

Com_Status Inf_FPM383_DelFingerPrint(uint16_t id)
{
    // 1. 准备指令集
    uint8_t cmd[16] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x07, // 包长度
        0x0c, // 指令码
        '\0',
        '\0', // 页码 PageID
        0x00,
        0x01, // 删除个数
        '\0',
        '\0', // 校验和
    };

    // 2. 补充校验和
    Inf_FPM383_AddChecksum(cmd, 16);

    // 3. 发送指令集
    Inf_FPM383_SendData(cmd, 16);

    // 4. 接收应答包 由于参数设置为不反回中间状态的应答包 所以只需要接收一次
    Inf_FPM383_RecvData(12, 2000);

    // 5. 解析应答包 判断是否成功
    if (rx_buf[9] == 0x00) // 成功
    {
        return Com_OK;
    }
    else
    {
        return Com_ERROR;
    }
}

void Inf_FPM383_DelAllFingerPrint(void)
{
    // 1. 准备指令集
    uint8_t cmd[12] = {
        0xEF,
        0x01, // 包头
        0xFF,
        0xFF,
        0xFF,
        0xFF, // 设备地址
        0x01, // 包标识
        0x00,
        0x03, // 包长度
        0x0d, // 指令码
        0x00,
        0x11, // 校验和
    };

    do
    {
        // 2. 发送指令集
        Inf_FPM383_SendData(cmd, 12);

        // 3. 接收应答包 由于参数设置为不反回中间状态的应答包 所以只需要接收一次
        Inf_FPM383_RecvData(12, 2000);
    } while (rx_buf[9] != 0x00);

    // 清空完成
    MY_LOGE("清空指纹库完成");
}
