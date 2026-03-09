#include "App_Communication.h"

#define HASH_LEN 32

/**
 * @brief 从服务器下载SHA256哈希文件
 * @param url 哈希文件URL
 * @param hash 输出缓冲区（32字节）
 */
static esp_err_t download_sha256_from_server(const char *url, uint8_t *hash)
{
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
        .buffer_size = 64,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        esp_http_client_cleanup(client);
        return err;
    }

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length != 32)
    {
        ESP_LOGE("ota", "哈希文件大小异常: %d", content_length);
    }

    int read_len = esp_http_client_read(client, (char *)hash, 32);
    esp_http_client_cleanup(client);

    return (read_len == 32) ? ESP_OK : ESP_FAIL;
}

static void get_sha256_of_partitions(void)
{
    uint8_t sha_256[32] = {0};

    // 打印bootloader的sha256
    esp_partition_t bootloader = {
        .address = ESP_BOOTLOADER_OFFSET,
        .size = ESP_PARTITION_TABLE_OFFSET,
        .type = ESP_PARTITION_TYPE_APP,
    };
    if (esp_partition_get_sha256(&bootloader, sha_256) == ESP_OK)
    {
        ESP_LOGI("ota", "Bootloader SHA256: %02x%02x...", sha_256[0], sha_256[1]);
    }

    // 打印当前运行分区的sha256
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running && esp_partition_get_sha256(running, sha_256) == ESP_OK)
    {
        ESP_LOGI("ota", "Running[%s] SHA256: %02x%02x...",
                 running->label, sha_256[0], sha_256[1]);
    }
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
        .url = "http://192.168.0.102:8080/hello_world.bin",
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = NULL,
        .keep_alive_enable = true,
        .timeout_ms = 10000,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota_handle_t ota_handle;

    /* 4. 开始OTA（改用分步式接口）*/
    err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("ota", "OTA开始失败");
        return;
    }

    /* 5. 分片下载 */
    while (1)
    {
        err = esp_https_ota_perform(ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            break;
        }
        printf("."); // 进度指示
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    printf("\n");

    /* 6. ✨✨✨ 关键检查1：是否完整接收 ✨✨✨ */
    if (esp_https_ota_is_complete_data_received(ota_handle) != true)
    {
        ESP_LOGE("ota", "下载不完整，取消升级");
        esp_https_ota_abort(ota_handle);
        return;
    }

    /* 7. ✨✨✨ 关键检查2：获取更新后的分区 ✨✨✨ */
    const esp_partition_t *updated_partition = esp_ota_get_next_update_partition(NULL);
    if (updated_partition == NULL)
    {
        ESP_LOGE("ota", "获取更新分区失败");
        esp_https_ota_abort(ota_handle);
        return;
    }

    /* 8. ✨✨✨ 关键检查3：计算SHA256并比对 ✨✨✨ */
    uint8_t calculated_sha[32] = {0};
    err = esp_partition_get_sha256(updated_partition, calculated_sha);
    if (err != ESP_OK)
    {
        ESP_LOGE("ota", "计算哈希失败");
        esp_https_ota_abort(ota_handle);
        return;
    }

    // 从服务器下载预期的哈希值
    uint8_t expected_sha[32] = {0};
    err = download_sha256_from_server("http://192.168.0.102:8080/hello_world.bin.sha256",
                                      expected_sha);
    if (err == ESP_OK)
    {
        // 比对哈希
        if (memcmp(calculated_sha, expected_sha, 32) != 0)
        {
            ESP_LOGE("ota", "SHA256校验失败！");
            esp_https_ota_abort(ota_handle);
            return;
        }
        ESP_LOGE("ota", "SHA256校验通过 ✓");
    }
    else
    {
        ESP_LOGE("ota", "未获取到预期哈希，仅依赖基础校验");
    }

    /* 9. 完成OTA */
    err = esp_https_ota_finish(ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("ota", "OTA完成失败");
        return;
    }

    /* 10. 设置启动分区（显式调用更安全）*/
    err = esp_ota_set_boot_partition(updated_partition);
    if (err != ESP_OK)
    {
        ESP_LOGE("ota", "设置启动分区失败");
        return;
    }

    ESP_LOGE("ota", "OTA成功，准备重启");
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
          D:\test\ota_test python -m http.server 8080
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
