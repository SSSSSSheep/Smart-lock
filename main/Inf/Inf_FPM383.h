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

/**
 * @brief 获取最小的指纹ID
 *
 * @return uint16_t 最小的指纹ID
 */
uint16_t Inf_FPM383_GetMinId(void);

/**
 * @brief 取消自动操作
 */
void Inf_FPM383_CancelAutoAction(void);

/**
 * @brief 添加指纹
 */
Com_Status Inf_FPM383_AddFingerPrint(uint16_t id);

/**
 * @brief 检查指纹
 */
Com_Status Inf_FPM383_CheckFingerPrint(void);

/**
 * @brief 查找指定ID指纹
 * @return int 指纹ID 存在返回ID 不存在返回-1
 */
int Inf_FPM383_FindFingerPrint(void);

/**
 * @brief 删除指定ID指纹
 * @param id 指纹ID
 */
Com_Status Inf_FPM383_DelFingerPrint(uint16_t id);

/**
 * @brief 删除所有指纹
 */
void Inf_FPM383_DelAllFingerPrint(void);

#endif // !__INF_FPM