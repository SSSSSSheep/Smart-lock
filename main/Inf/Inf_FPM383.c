#include "Inf_FPM383.h"

// 接收缓冲区大小
static const int RX_BUF_SIZE = 1024;

static uint8_t rx_buf[100] = {1};

uint8_t hasFinger = 0;
static void IRAM_ATTR Inf_FPM383_Intr_Handler(void *args)
{
    // 防抖处理：检查中断引脚的状态
    static uint32_t last_interrupt_time = 0;
    uint32_t current_time = esp_timer_get_time();

    // 忽略短时间内的重复中断（防抖）
    if (current_time - last_interrupt_time > 100000)
    { // 100ms防抖
        hasFinger = 1;
        last_interrupt_time = current_time;
    }
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

/**
 * @brief 诊断指纹模组通信问题
 * @return 诊断结果，0表示正常，非0表示有问题
 */
int Inf_FPM383_Diagnose(void)
{
    MY_LOGE("开始诊断指纹模组通信问题...");

    // 1. 测试串口通信
    MY_LOGE("1. 测试串口通信...");
    uint8_t test_cmd[12] = {
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

    // 发送测试指令
    int send_len = uart_write_bytes(UART_NUM_1, test_cmd, 12);
    if (send_len != 12)
    {
        MY_LOGE("错误: 串口发送失败，只发送了 %d 字节", send_len);
        return 1;
    }
    MY_LOGE("串口发送成功，发送了 12 字节");

    // 接收响应
    memset(rx_buf, 0, sizeof(rx_buf));
    vTaskDelay(pdMS_TO_TICKS(100)); // 等待响应
    int recv_len = uart_read_bytes(UART_NUM_1, rx_buf, sizeof(rx_buf), 1000);

    if (recv_len <= 0)
    {
        MY_LOGE("错误: 未接收到响应");
        return 2;
    }

    MY_LOGE("接收到 %d 字节响应:", recv_len);
    for (int i = 0; i < recv_len; i++)
    {
        MY_LOGE("0x%02X ", rx_buf[i]);
    }
    MY_LOGE("");

    // 2. 检查响应格式
    MY_LOGE("2. 检查响应格式...");
    if (recv_len < 12)
    {
        MY_LOGE("错误: 响应长度不足，至少需要 12 字节");
        return 3;
    }

    if (rx_buf[0] != 0xEF || rx_buf[1] != 0x01)
    {
        MY_LOGE("错误: 响应包头不正确");
        return 4;
    }

    if (rx_buf[9] != 0x00 && rx_buf[9] != 0x01)
    {
        MY_LOGE("错误: 响应状态码不正确，期望 0x00 或 0x01，实际 0x%02X", rx_buf[9]);
        return 5;
    }
    else if (rx_buf[9] == 0x01)
    {
        MY_LOGE("注意: 模组已经处于休眠状态");
    }

    // 3. 测试休眠指令
    MY_LOGE("3. 测试休眠指令...");
    int retry = 0;
    const int MAX_RETRY = 3;
    bool sleep_success = false;

    do
    {
        MY_LOGE("尝试进入休眠模式... (%d/%d)", retry + 1, MAX_RETRY);

        // 发送休眠指令
        Inf_FPM383_SendData(test_cmd, 12);

        // 接收响应
        memset(rx_buf, 0, sizeof(rx_buf));
        int sleep_recv = uart_read_bytes(UART_NUM_1, rx_buf, 12, 2000);

        if (sleep_recv == 12)
        {
            MY_LOGE("接收到休眠响应:");
            for (int i = 0; i < 12; i++)
            {
                MY_LOGE("0x%02X ", rx_buf[i]);
            }
            MY_LOGE("");

            if (rx_buf[9] == 0x00 || rx_buf[9] == 0x01)
            {
                MY_LOGE("休眠指令执行成功!");
                sleep_success = true;
                break;
            }
            else
            {
                MY_LOGE("休眠指令执行失败，状态码: 0x%02X", rx_buf[9]);
            }
        }
        else
        {
            MY_LOGE("未接收到完整的休眠响应，只收到 %d 字节", sleep_recv);
        }

        retry++;
        vTaskDelay(pdMS_TO_TICKS(500));
    } while (retry < MAX_RETRY);

    if (!sleep_success)
    {
        MY_LOGE("错误: 无法进入休眠模式");
        return 6;
    }

    // 4. 测试中断功能
    MY_LOGE("4. 测试中断功能...");
    gpio_intr_enable(Inf_FPM383_INTR_PIN);
    MY_LOGE("中断已启用，等待指纹触发...");

    // 等待一段时间看是否有中断触发
    vTaskDelay(pdMS_TO_TICKS(5000));

    if (hasFinger)
    {
        MY_LOGE("中断功能正常，检测到指纹");
        hasFinger = 0; // 重置标志
    }
    else
    {
        MY_LOGE("警告: 未检测到指纹中断，可能需要手动测试");
    }

    MY_LOGE("诊断完成，所有测试通过!");
    return 0;
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

    // 2. 禁用中断，防止在休眠过程中被触发
    gpio_intr_disable(Inf_FPM383_INTR_PIN);

    // 3.发送指令进入休眠模式
    int retry = 0;
    const int MAX_RETRY = 3;
    bool sleep_success = false;

    do
    {
        MY_LOGE("即将进入休眠模式... (尝试 %d/%d)", retry + 1, MAX_RETRY);
        // 3.1 发送指令
        Inf_FPM383_SendData(cmd, 12);

        // 3.2 接收应答包
        Com_Status status = Inf_FPM383_RecvData(12, 2000);

        MY_LOGE("接收状态: %d", status);
        if (status == Com_OK)
        {
            MY_LOGE("接收到的数据:");
            for (int i = 0; i < 12; i++)
            {
                MY_LOGE("0x%02X ", rx_buf[i]);
            }
            MY_LOGE("");

            // 检查响应
            if (rx_buf[0] == 0xEF && rx_buf[1] == 0x01 && // 包头
                rx_buf[6] == 0x07)
            { // 包标识

                if (rx_buf[9] == 0x00)
                {
                    // 休眠成功
                    sleep_success = true;
                    MY_LOGE("休眠指令执行成功!");
                    break;
                }
                else if (rx_buf[9] == 0x01)
                {
                    // 已经处于休眠状态
                    sleep_success = true;
                    MY_LOGE("模组已经处于休眠状态!");
                    break;
                }
                else
                {
                    MY_LOGE("休眠指令执行失败，状态码: 0x%02X", rx_buf[9]);
                }
            }
            else
            {
                MY_LOGE("响应格式不正确");
            }
        }

        retry++;
        vTaskDelay(pdMS_TO_TICKS(300));
    } while (retry < MAX_RETRY);

    // 4. 开启中断 发送任意指令可以唤醒模块
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

    // 2. 补充ID和校验和
    cmd[10] = (id >> 8);
    cmd[11] = id;
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