#ifndef __DRI_NVS_H__
#define __DRI_NVS_H__

#include <stdio.h>
#include <inttypes.h>
#include "esp_task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "Com_Config.h"
#include "string.h"

#define MY_NVS_NAMESPACE "pwd"

extern nvs_handle_t my_nvs_handle;

void Dri_NVS_Init(void);

esp_err_t Dri_NVS_WriteU8(char *k, uint8_t v);

esp_err_t Dri_NVS_ReadU8(char *k, uint8_t *v);

Com_Status Dri_NVS_IsKeyExist(uint8_t *key, uint8_t isFull);

#endif
