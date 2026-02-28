#ifndef __DRI_NVS_H__
#define __DRI_NVS_H__

#include <stdio.h>
#include <inttypes.h>
#include "nvs_flash.h"
#include "nvs.h"

void Dri_NVS_Init(void);

/**
 * @brief 写入字符串到 NVS
 *
 * @param key 键名
 * @param value 键值
 */
esp_err_t Dri_NVS_WriteStr(uint8_t *key, uint8_t *value);

/**
 * @brief 从 NVS 读取字符串
 *
 * @param key 键名
 * @param value 键值
 * @param value_len 键值长度
 */
esp_err_t Dri_NVS_ReadStr(uint8_t *key, uint8_t *value, uint8_t *value_len);

/**
 * @brief 删除 NVS 中的指定键
 *
 * @param key 键名
 */
esp_err_t Dri_NVS_DelKey(uint8_t *key);

/**
 * @brief 删除 NVS 中的所有键
 *
 */
esp_err_t Dri_NVS_DelAll(void);

/**
 * @brief 检查 NVS 中是否存在指定键
 *
 * @param key 键名
 * @return esp_err_t ESP_OK 成功, 其他值失败
 */
esp_err_t Dri_NVS_IsKeyExist(uint8_t *key);

#endif /* __DRI_NVS_H__ */
