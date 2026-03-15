#include "Dri_NVS.h"

/* nvs句柄 */
nvs_handle_t my_nvs_handle;
/**
 * @description: NVS 存储初始化
 * @return {*}
 */
void Dri_NVS_Init(void)
{
    /* 1. 初始化nvs */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        nvs_flash_erase();
        err = nvs_flash_init();
    }

    /* 2. 打开nvs */
    nvs_open(MY_NVS_NAMESPACE, NVS_READWRITE, &my_nvs_handle);
}

/**
 * @description: 向nvs中写入指定的键值对  v是u8类型
 * @param {char} *k
 * @param {uint8_t} v
 * @return {*}
 */
esp_err_t Dri_NVS_WriteU8(char *k, uint8_t v)
{
    /* 如果已经存在, 则直接返回存储失败 */
    if (nvs_find_key(my_nvs_handle, k, NULL) == ESP_OK)
        return ESP_FAIL;

    nvs_set_u8(my_nvs_handle, k, v);
    return nvs_commit(my_nvs_handle);
}

/**
 * @description: 读取u8
 * @param {char} *k
 * @param {uint8_t} *v
 * @return {*}
 */
esp_err_t Dri_NVS_ReadU8(char *k, uint8_t *v)
{
    return nvs_get_u8(my_nvs_handle, k, v);
}

/**
 * @description: 判断指定的key是否存在
 *      isFull  输入的密码    存储的密码      返回值
 *        1       123          123            ok
 *        0       123          123            error
 *
 *        1       01234        123            erro
 *        0       01234        123            ok
 * @param {uint8_t} *key
 * @param {uint8_t} isFull
 * @return {*}
 */
Com_Status Dri_NVS_IsKeyExist(uint8_t *key, uint8_t isFull)
{
    nvs_iterator_t it;
    /* 通过迭代器的方式遍所有key */
    esp_err_t res = nvs_entry_find_in_handle(my_nvs_handle, NVS_TYPE_ANY, &it);
    while (res == ESP_OK)
    {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        if (isFull == 1)
        {
            if (strcmp((char *)key, info.key) == 0) /* 完全相同 当比较结果为0时 */
            {
                return Com_OK;
            }
        }
        else
        {
            if (strstr((char *)key, info.key) != NULL) /* 存储的密码是输入密码的一部分 */
            {
                return Com_OK;
            }
        }

        res = nvs_entry_next(&it);
    }

    return Com_ERROR;
}