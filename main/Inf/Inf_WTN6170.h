#ifndef __INF_WTN6170_H__
#define __INF_WTN6170_h__

#include "driver/gpio.h"
#include "sys/unistd.h"
#include "esp_task.h"

#define WTN6170_DATA_PIN GPIO_NUM_9
#define WTN6170_DATA_L (gpio_set_level(WTN6170_DATA_PIN, 0))
#define WTN6170_DATA_H (gpio_set_level(WTN6170_DATA_PIN, 1))

// 语音模块初始化
void Inf_WTN6170_Init(void);

/**
 * @description: 发送指令
 * @param: {uint8_t} cmd指令
 */
void Inf_WTN6170_SendCmd(uint8_t cmd);

#endif // !__INF_WTN6170_H__
