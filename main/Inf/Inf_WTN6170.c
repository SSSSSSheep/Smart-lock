#include "Inf_WTN6170.h"

void Inf_WTN6170_Init(void)
{
    // GPIO配置信息
    gpio_config_t io_config = {};
    io_config.intr_type = GPIO_INTR_DISABLE;
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pull_down_en = 0;
    io_config.pull_up_en = 0;
    io_config.pin_bit_mask = (1 << WTN6170_DATA_PIN);

    // 让配置信息生效
    gpio_config(&io_config);

    // 默认让数据线拉高
    WTN6170_DATA_H;
}

void Inf_WTN6170_SendCmd(uint8_t cmd)
{
    // 数据线拉低10ms
    WTN6170_DATA_L;
    vTaskDelay(10);
    // 循环发送每一位 低位在前
    for (uint8_t i = 0; i < 8; i++)
    {
        if (cmd & 0x01)
        {
            // 数据为1 先拉高 在拉低 高低比例3：1
            WTN6170_DATA_H;
            usleep(600);
            WTN6170_DATA_L;
            usleep(200);
        }
        else
        {
            // 数据为0 先拉高 在拉低 高低比例1：3
            WTN6170_DATA_H;
            usleep(200);
            WTN6170_DATA_L;
            usleep(600);
        }

        // 将数据右移
        cmd >>= 1;
    }
    // 数据线拉高
    WTN6170_DATA_H;
    vTaskDelay(5);
}
