#ifndef __INF_FPM383_H__
#define __INF_FPM383_H__

#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "Com_Debug.h"
#include "Com_Config.h"

#define Inf_FPM383_TX_PIN GPIO_NUM_21
#define Inf_FPM383_RX_PIN GPIO_NUM_20
#define Inf_FPM383_INTR_PIN GPIO_NUM_10

/**
 * @brief 初始化FPM383模块
 *
 */
void Inf_FPM383_Init(void);

/**
 * @brief 进入休眠模式
 */
void Inf_FPM383_Sleep(void);

/**
 *
 */
void Inf_FPM383_ReadId(void);

#endif // !__INF_FPM