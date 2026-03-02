#include "App_Communication.h"

#define HASH_LEN 32
static void get_sha256_of_partitions(void)
{
    uint8_t sha_256[HASH_LEN] = {0};
    esp_partition_t partition;

    // get sha256 digest for bootloader
    partition.address = ESP_BOOTLOADER_OFFSET;
    partition.size = ESP_PARTITION_TABLE_OFFSET;
    partition.type = ESP_PARTITION_TYPE_APP;
    esp_partition_get_sha256(&partition, sha_256);

    // get sha256 digest for running partition
    esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
}

#define TAG "ota"
/// 处理一系列的HTTP事件
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)

    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

/**
 * @description: 下载ota用的二进制文件
 * @return {*}
 */
static void App_Communication_OTADownloadBin(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)

    {
        nvs_flash_erase();
        err = nvs_flash_init();
    }

    /* 1. 获取分区信息 */
    get_sha256_of_partitions();

    /* 2. 初始化网络 */
    esp_netif_init();

    /* 3. 创建和初始化默认事件循环 */
    esp_event_loop_create_default();

    esp_http_client_config_t config = {
        .url = "http://192.168.0.101:8080/hello_world.bin",
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = NULL,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota(&ota_config);
}

/**
 * @brief 添加密码
 * @param pwd 密码指针
 */
static void App_Communication_AddPwd(uint8_t *pwd)
{
    Com_Status comStatus = Dri_NVS_WriteStr(pwd, (uint8_t *)"0");
    if (comStatus == ESP_OK)
    {
        sayAddSucc();
    }
    else
    {
        sayAddFail();
    }
}

/**
 * @brief 删除密码
 * @param pwd 密码指针
 */
static void App_Communication_DelPwd(uint8_t *pwd)
{
    if (Dri_NVS_IsKeyExist(pwd) != ESP_OK)
    {
        sayDelFail();
    }
    else
    {
        esp_err_t err = Dri_NVS_DelKey(pwd);
        if (err == ESP_OK)
        {
            sayDelSucc();
        }
        else
        {
            sayDelFail();
        }
    }
}

/**
 * @brief 验证密码开锁
 * @param pwd 密码指针
 */
static void App_Communication_CheckPwd(uint8_t *pwd)
{
    if (Dri_NVS_IsKeyExist(pwd) != ESP_OK)
    {
        sayVerifyFail();
    }
    else
    {
        sayVerifySucc();
        Inf_DBR6120_OpenLock();
        sayDoorOpen();
    }
}

void App_Communication_Init(void)
{
    Dri_BLE_Init();
}

void App_Communication_OTA(void)
{
    /* 1. 连接wifi */
    Dri_Wifi_Init();

    /* 2. ota升级   使用python启动个本地http-server 命令
          D:\test\ota_testpython -m http.server 8080
    */
    printf("ota开始升级\r\n");
    App_Communication_OTADownloadBin();
    printf("ota完成升级\r\n");

    /* 3. 关闭wifi */
    esp_wifi_stop();

    /* 4. 重启esp32 */
    esp_restart();
}

/**
 * @brief 处理esp32收到手机数据时的回调函数
 * @param data 收到的数据指针 设备号+操作类型+密码 xxx+0/添加1/删除2/验证+666666
 * @param dataLen 收到的数据长度
 * 添加密码
 * 删除密码
 * 验证密码开锁
 */
void App_Communication_RecvDataCb(uint8_t *data, uint16_t dataLen)
{
    printf("Value = %s,Len = %d\r\n", data, dataLen);

    // 获取手机端传输过来的数据中的密码
    uint8_t pwd[100] = {0};
    memcpy(pwd, &data[2], dataLen - 2);
    switch (data[0])
    {
    case '0': // 添加密码
        App_Communication_AddPwd(pwd);
        break;
    case '1': // 删除密码
        App_Communication_DelPwd(pwd);
        break;
    case '2': // 验证密码开锁
        App_Communication_CheckPwd(pwd);
        break;
    }
}
