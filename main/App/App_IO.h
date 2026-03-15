#ifndef __APP_IO_H__
#define __APP_IO_H__

#include "Inf_SC12B.h"
#include "Inf_WS2812.h"
#include "Dri_NVS.h"
#include "Inf_WTN6170.h"
#include "Inf_BDR6120.h"
#include "Inf_FPM383.h"
#include "esp_log.h"

/* 输入状态 */
typedef enum
{
    FREE = 0, /* 自由 */
    INPUT,    /* 输入阶段 */
    DONE      /* 输入完成 */
} Input_Status;

extern TaskHandle_t fingerprintScanTaskkHandle;
extern TaskHandle_t communicationHandle;

void App_IO_Start(void);

void App_IO_KeyScan(void);

void App_IO_AddPwd(uint8_t *pwd, uint8_t pwdLen);

void App_IO_CheckPwd(uint8_t *pwd);

void App_IO_DelPwd(uint8_t *pwd);

void App_IO_FingerPrintScan(void);

#endif
