#ifndef __APP_IO_H__
#define __APP_IO_H__

#include "Dri_NVS.h"
#include "Inf_SC12B.h"
#include "Inf_WS2812.h"
#include "Inf_DBR6120.h"
#include "Inf_WTN6170.h"
#include "Inf_FPM383.h"
#include "Com_Config.h"
#include "App_Communication.h"

#define ADMIN_PWD "admin"

/**
 * @brief 初始化IO模块
 */
void App_IO_Init(void);

/**
 * @brief 从IO模块读取字符串
 * @param pwd 存储读取到的密码的数组
 * @return Com_Status 读取状态
 */
Com_Status App_IO_ReadStr(uint8_t *pwd);

/**
 * @brief 处理IO模块读取到的密码
 * @param pwd 存储读取到的密码的数组
 */
void App_IO_Handler(uint8_t *pwd);

/**
 * @brief 处理指纹模块读取到的指纹
 */
void App_IO_Finger(void);

#endif // !__APP_IO_H__
