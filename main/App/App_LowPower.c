/*
低功耗设计:
    1. 指纹模块
            进入低功耗:  睡眠模式
            退出低功耗:  有手指触摸指纹

    2. 触摸按键感应器
            进入低功耗: 在一段时间内（75秒左右）没有检测到按键并且SDA端口一直保持高电平
            退出低功耗: 检测到按键，芯片会马上离开睡眠模式，进入正常模式

    3.  语音模块
            进入低功耗: 语音播放完成后，DATA /CLK信号线保持稳定的电平2秒左右即可休眠
            退出低功耗: 当有语音时, 自动退出低功耗

    4.  wifi
            当需要OTA的时候打开wifi, OTA结束之后关闭

    5. BLE
            低功耗蓝牙

    6. esp32
        https://docs.espressif.com/projects/esp-idf/zh_CN/stable/esp32/api-guides/low-power-mode.html
        DFS
            DFS (Dynamic frequency scaling) 即动态频率切换，是 ESP-IDF 中集成的电源管理机制的基础功能。
            DFS 可以根据应用程序持有电源锁的情况，调整外围总线 (APB) 频率和 CPU 频率。
            持有高性能锁就使用高频，空闲状态不持有电源锁时则使用低频来降低功耗，以此来尽可能减少运行应用程序的功耗。
        Light-sleep
            a:Light-sleep 模式是 ESP32 预设的一种低功耗模式，其核心思想就是在休眠时关闭或门控一些功能模块来降低功耗。
            b:需要配置唤醒源进行唤醒
            c:Auto Light-sleep 模式是 ESP-IDF 电源管理机制和 Light-sleep 模式的结合。
             开启电源管理机制是其前置条件，auto 体现在系统进入空闲状态 (IDLE) 超过设定时间后，自动进入 Light-sleep。
             空闲状态下，应用程序释放所有电源锁，此时，DFS 将降频以减小功耗。
            d:休眠时会自动关闭 RF、8 MHz 振荡器、40 MHz 高速晶振、PLL、门控数字内核时钟，暂停 CPU 工作。
            e:具体唤醒源有 RTC 定时器、触摸传感器、外部唤醒 (ext0)、外部唤醒 (ext1)、
                ULP 协处理器、SDIO、GPIO、UART、Wi-Fi、BT 唤醒等。

            Auto Light-sleep 模式适用于不需要实时响应外界需求的场景
        Deep-sleep
            a: Deep-sleep 模式是为了追求更好的功耗表现所设计，
            b: 休眠时仅保留 RTC 控制器、RTC 外设（可配置）、ULP 协处理器、RTC 高速内存、RTC 低速内存，其余模块全部关闭。
            c: 与 Light-sleep 类似，Deep-sleep 同样通过 API 进入，且需要配置唤醒源进行唤醒。
            d: 具体唤醒源有 RTC 定时器、触摸传感器、外部唤醒 (ext0)、外部唤醒 (ext1)、ULP 协处理器、GPIO 唤醒等。

            Deep-sleep 可以用于低功耗的传感器应用，或是大部分时间都不需要进行数据传输的情况，也就是通常所说的待机模式。
            设备可以每隔一段时间从 Deep-sleep 状态醒来测量数据并上传，之后重新进入 Deep-sleep；
            也可以将多个数据存储于 RTC memory，然后一次性发送出去。


        如需保持 Wi-Fi 和 Bluetooth 连接，请启用 Wi-Fi 和 Bluetooth Modem-sleep 模式和自动 Light-sleep 模式（请参阅 电源管理 API）。
        在这两种模式下，Wi-Fi 和 Bluetooth 驱动程序发出请求时，系统将自动从睡眠中被唤醒，从而保持连接。;
*/

#include "App_LowPower.h"
#include "Inf_SC12B.h"
#include "Inf_WS2812.h"
#include "Inf_WTN6170.h"
#include "Inf_BDR6120.h"
#include "Inf_FPM383.h"
#include "Dri_NVS.h"
#include "Dri_BLE.h"

/**
 * @description: 低功耗模式初始化
 *  在低功耗的时候,我们需要保持蓝牙开启状态
 *
 *  什么时候进入低功耗:
 *
 *
 *  如何唤醒低功耗:
 *      1.  触摸按键有触摸
 *      2.  指纹模块收到指纹
 * @return {*}
 */
void App_LowPower_Enter(void)
{
    /* 1.配置唤醒源 */
    gpio_wakeup_enable(GPIO_NUM_10, ESP_GPIO_WAKEUP_GPIO_HIGH);
    gpio_wakeup_enable(GPIO_NUM_0, ESP_GPIO_WAKEUP_GPIO_HIGH);

    esp_sleep_enable_gpio_wakeup();

    /* 进入轻睡眠模式 */
    esp_light_sleep_start();

    /* 从睡眠模式唤醒后，重新初始化所有必要的模块 */
    /* 1. 触摸感应器初始化 */
    Inf_SC12B_Init();
    /* 2. LED初始化 */
    Inf_WS2812_Init();
    /* 3. nvs初始化 */
    Dri_NVS_Init();
    /* 4. 初始化语音输出模块 */
    Inf_WTN6170_Init();
    /* 5. 电机驱动芯片初始化 */
    Inf_BDR6120_Init();
    /* 6. 指纹初始化 */
    Inf_FPM383_Init();
    /* 7. 蓝牙初始化 */
    Inf_BLE_Init();
}