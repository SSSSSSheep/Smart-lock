#include "App_Communication.h"

void App_Communication_Start(void)
{
    Inf_BLE_Init();
}

/* 覆盖 蓝牙收到数据时 的弱回调函数*/
void App_Communication_RecvDataCb(uint8_t *data, uint16_t dataLen)
{
    MY_LOGE("收到ble数据 len=%d, data=%.*s\r\n", dataLen, dataLen, data);
    /*
        蓝牙发送数据格式:     功能
                1:           开锁
                2:密码       设置密码
                3:密码       删除密码
    */
    /* 1. 数据长度 < 2, 直接返回, 没有任何操作 */
    if (dataLen < 2)
        return;
    /*
        客户端连接上蓝牙之后, 会发送锁的 序列号 +open 来开锁
            锁的序列号一般在锁出厂的时候就已经固定了,而且是唯一的

            我们可以使用 esp32的mac地址作为序列号

     */

    /* 2. 判断操作类型 */
    uint8_t pwd[20];
    switch (data[0])
    {
    case '1': /* 开锁 */
    {
        Inf_BDR6120_LockOpen();
        sayDoorOpen(); /* 开门: 音效 */
        MY_LOGE("门已开");
        break;
    }
    case '2': /* 设置密码 */
    {
        memcpy(pwd, &data[2], dataLen - 2);
        App_IO_AddPwd(pwd, dataLen - 2);
        break;
    }
    case '3': /* 删除密码 */
    {
        memcpy(pwd, &data[2], dataLen - 2);
        App_IO_DelPwd(pwd);
        break;
    }

    default:
        break;
    }
}

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
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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
        .url = "http://192.168.11.91:8080/smart_lock_4.3.bin",
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
 * @description: 实现OTA操作
 * @return {*}
 */
void App_Communication_OTA(void)
{
    /* 1. 连接wifi */
    Inf_Wifi_Init();

    /* 2. ota升级   使用python启动个本地http-server 命令
          C:\esp\tools\idf-python\3.11.2\python -m http.server 8080
    */
    printf("ota开始升级\r\n");
    App_Communication_OTADownloadBin();
    printf("ota完成升级\r\n");

    /* 3. 关闭wifi */
    esp_wifi_stop();

    /* 4. 重启esp32 */
    esp_restart();
}
