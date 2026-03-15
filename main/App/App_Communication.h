#ifndef __APP_COMMUNICATION_H__
#define __APP_COMMUNICATION_H__

#include "Dri_BLE.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "Inf_WS2812.h"
#include "Inf_WTN6170.h"
#include "Inf_BDR6120.h"
#include "Dri_Wifi.h"

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "Com_Debug.h"
#include "App_IO.h"

void App_Communication_Start(void);
void App_Communication_OTA(void);

#endif
