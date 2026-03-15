/*
    语音交互模块

*/
#include "Inf_WTN6170.h"

/**
 * @description: 语音模块的初始化
 * @return {*}
 */
void Inf_WTN6170_Init(void)
{
    gpio_config_t conf;
    conf.intr_type = GPIO_INTR_DISABLE; /* 禁用中断 */
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.pull_up_en = GPIO_PULLUP_DISABLE;
    conf.pin_bit_mask = 1ULL << WTN6170_SDA;

    gpio_config(&conf);
}

/**
 * @description: 发送语音命令
 * @param {uint8_t} cmd 语音命令
 * @return {*}
 */
void Inf_WTN6170_SendCmd(uint8_t cmd)
{

    WTN6170_SDA_L;
    /* 关键延时 */
    delay_ms(10);
    for (uint8_t i = 0; i < 8; i++)
    {
        if (cmd & 0x1)
        {
            WTN6170_SDA_H;
            delay_us(600);
            WTN6170_SDA_L;
            delay_us(200);
        }
        else
        {
            WTN6170_SDA_H;
            delay_us(200);
            WTN6170_SDA_L;
            delay_us(600);
        }
        cmd >>= 1;
    }
    WTN6170_SDA_H;
    delay_ms(4);
}
