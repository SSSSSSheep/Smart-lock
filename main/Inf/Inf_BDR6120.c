/* 电机驱动芯片  */
#include "Inf_BDR6120.h"

#define BDR_INA GPIO_NUM_5
#define BDR_INB GPIO_NUM_4
/**
 * @description: 初始化
 * @return {*}
 */
void Inf_BDR6120_Init(void)
{
    /*  配置INA和INB引脚的工作模式 */
    gpio_config_t conf;
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.pull_up_en = GPIO_PULLUP_DISABLE;
    conf.pin_bit_mask = (1 << BDR_INA) | (1 << BDR_INB);

    gpio_config(&conf);

    Inf_BDR6120_Brake();
}

/**
 * @description: 驱动马达正转
 *
 * @return {*}
 */
void Inf_BDR6120_ForwardRotation(void)
{
    gpio_set_level(BDR_INA, 1);
    gpio_set_level(BDR_INB, 0);
}

/**
 * @description: 驱动马达反转
 * @return {*}
 */
void Inf_BDR6120_ReverseRotation()
{
    gpio_set_level(BDR_INA, 0);
    gpio_set_level(BDR_INB, 1);
}

/**
 * @description: 刹车
 * @return {*}
 */
void Inf_BDR6120_Brake()
{
    gpio_set_level(BDR_INA, 1);
    gpio_set_level(BDR_INB, 1);
}

/**
 * @description: 开锁
 * @return {*}
 */
void Inf_BDR6120_LockOpen(void)
{
    /* 正转1s */
    Inf_BDR6120_ForwardRotation();
    vTaskDelay(1000);
    /* 停止1s */
    Inf_BDR6120_Brake();
    vTaskDelay(1000);
    /* 反转1s */
    Inf_BDR6120_ReverseRotation();
    vTaskDelay(1000);
    /* 停止 */
    Inf_BDR6120_Brake();
}
