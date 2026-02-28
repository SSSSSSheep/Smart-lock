#include "Dri_NVS.h"

// 定义一个 NVS 句柄
static nvs_handle_t my_handle;

void Dri_NVS_Init(void)
{
    // 初始化 NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    // 打开一块命名空间
    nvs_open("LockDate", NVS_READWRITE, &my_handle);
}

esp_err_t Dri_NVS_WriteStr(uint8_t *key, uint8_t *value)
{
    return nvs_set_str(my_handle, (char *)key, (char *)value);
}

esp_err_t Dri_NVS_ReadStr(uint8_t *key, uint8_t *value, size_t *value_len)
{
    return nvs_get_str(my_handle, (char *)key, (char *)value, value_len);
}

esp_err_t Dri_NVS_DelKey(uint8_t *key)
{
    return nvs_erase_key(my_handle, (char *)key);
}

esp_err_t Dri_NVS_DelAll(void)
{
    return nvs_erase_all(my_handle);
}

esp_err_t Dri_NVS_IsKeyExist(uint8_t *key)
{
    return nvs_find_key(my_handle, (char *)key, NULL);
}
