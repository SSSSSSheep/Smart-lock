#include "Inf_FPM383.h"

// 接收缓冲区大小
static const int RX_BUF_SIZE = 1024;

static uint8_t rx_buf[100] = {1};

uint8_t hasFinger = 0;
static void Inf_FPM383_Intr_Handler(void *args)
{
    esp_rom_printf("Inf_FPM383_Intr_Handler\n");
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
