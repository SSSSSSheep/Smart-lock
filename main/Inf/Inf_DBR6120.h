#ifndef __INF_BDR6120_H__
#define __INF_BDR6120_H__

#include "driver/gpio.h"
#include "Com_Debug.h"
#include "esp_task.h"

#define INF_BDR_INA GPIO_NUM_5
#define INF_BDR_INB GPIO_NUM_4

/**
 * @brief 初始化DBR6120电机驱动
 *
 */
void Inf_DBR6120_Init(void);

/**
 * @brief 电机正转
 *
 */
void Inf_DBR6120_Forward(void);
/**
 * @brief 电机反转
 *
 */
void Inf_DBR6120_Backward(void);
/**
 * @brief 打开锁
 *
 */
void Inf_DBR6120_OpenLock(void);

#endif // !__INF_BDR6120_H__
