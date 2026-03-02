#ifndef __APP_COMMUNICATION_H__
#define __APP_COMMUNICATION_H__

#include "Dri_BLE.h"
#include "Dri_NVS.h"
#include "Inf_WTN6170.h"
#include "Inf_DBR6120.h"
#include "Com_Config.h"
#include "Dri_Wifi.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "string.h"
#include "esp_crt_bundle.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <sys/socket.h>

/**
 * @brief 初始化通信模块
 *
 */
void App_Communication_Init(void);

/**
 * @brief OTA升级
 */
void App_Communication_OTA(void);

#endif