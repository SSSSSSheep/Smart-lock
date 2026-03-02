#ifndef __APP_COMMUNICATION_H__
#define __APP_COMMUNICATION_H__

#include "Dri_BLE.h"
#include "Dri_NVS.h"
#include "Inf_WTN6170.h"
#include "Inf_DBR6120.h"
#include "Com_Config.h"

/**
 * @brief 初始化通信模块
 *
 */
void App_Communication_Init(void);

#endif