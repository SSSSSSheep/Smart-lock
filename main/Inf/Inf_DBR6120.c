#include "Inf_DBR6120.h"

void Inf_DBR6120_Init(void)
{
    // GPIO配置项
    gpio_config_t io_config = {};
    io_config.intr_type = GPIO_INTR_DISABLE;
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    io_config.pin_bit_mask = (1 << INF_BDR_INA) | (1 << INF_BDR_INB);

    // 让配置信息生效
    gpio_config(&io_config);

    // 电机初始状态为停止
    gpio_set_level(INF_BDR_INA, 0);
    gpio_set_level(INF_BDR_INB, 0);
}

void Inf_DBR6120_Forward(void)
{
    gpio_set_level(INF_BDR_INA, 1);
    gpio_set_level(INF_BDR_INB, 0);
}

void Inf_DBR6120_Backward(void)
{
    gpio_set_level(INF_BDR_INA, 0);
    gpio_set_level(INF_BDR_INB, 1);
}

void Inf_DBR6120_Break(void)
{
    gpio_set_level(INF_BDR_INA, 1);
    gpio_set_level(INF_BDR_INB, 1);
}

void Inf_DBR6120_OpenLock(void)
{
    // 先正转1s
    Inf_DBR6120_Backward();

    vTaskDelay(1000);
    // 停止1s
    Inf_DBR6120_Break();
    vTaskDelay(1000);
    // 然后反转1s
    Inf_DBR6120_Forward();
    vTaskDelay(1000);

    Inf_DBR6120_Break();
}
