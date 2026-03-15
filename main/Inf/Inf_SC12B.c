#include "Inf_SC12B.h"

/*
利用 GPIO交换矩阵，
    可配置外设模块的输入信号来源于任何的 IO 管脚，
    并且外设模块的输出信号也可连接到任意 IO 管脚

根据原理图进行如下配置
*/
#define I2C_MASTER_SDA_IO 2       /* sda引脚定义 */
#define I2C_MASTER_SCL_IO 1       /* scl引脚定义 */
#define I2C_MASTER_FREQ_HZ 100000 /* i2c速率 */

#define SC12B_INT_IO 0 /* INT引脚定义 */

#define SC12B_I2C_ADDR (0x42)

static void Inf_SC12B_WriteReg(uint8_t reg, uint8_t value);

void scb12IntHandler(void *args);

/**
 * @description: 初始化触摸感应器芯片
 * @return {*}
 */
void Inf_SC12B_Init(void)
{
    /* 0. esp32-c3 只有一个i2c, 所以port只能是0 */
    int i2c_master_port = 0;

    /* 1. i2c配置结构体 */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER, /* 主模式 */
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        /* 外部没有上拉电阻时, 内部要有上拉电阻 */
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    /* 2. 配置i2c */
    i2c_param_config(i2c_master_port, &conf);

    /* 3. 安装i2c驱动
        参数3和参数4只有当slave时才有效, 我们是主模式,忽略
        参数5 是中断相关配置,0表示使用默认配置
    */
    i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);

    /* 4 给p0配置外部中断*/
    /* 4.1 给p0引脚配置中断模式 */
    gpio_config_t int_conf;
    int_conf.intr_type = GPIO_INTR_POSEDGE;       /* 上升沿触发 */
    int_conf.mode = GPIO_MODE_INPUT;              /* 输入模式 */
    int_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; /* 下拉 */
    int_conf.pin_bit_mask = 1 << SC12B_INT_IO;    /* 中断引脚 */
    gpio_config(&int_conf);
    /* 4.2 注册(安装)中断服务
       0 表示不需要额外的特殊配置。
        这意味着 ISR 服务会使用默认的中断分配策略（Level 1 中断、边沿触发，处理程序不放在 IRAM 中等）。
    */
    gpio_install_isr_service(0);
    /* 4.3 添加中断处理函数 */
    gpio_isr_handler_add(SC12B_INT_IO, scb12IntHandler, (void *)SC12B_INT_IO);

    /* 5. 灵敏度设置 : 降低灵敏度,主要是降低干扰,导致的误触*/
    Inf_SC12B_WriteReg(0x01, 0x15); // 增加灵敏度

    /* 6. 配置触摸按键模块 */
    Inf_SC12B_WriteReg(0x02, 0x00); // 配置工作模式
    Inf_SC12B_WriteReg(0x03, 0x00); // 配置低功耗模式
    Inf_SC12B_WriteReg(0x04, 0x00); // 配置中断模式
}

/* 表示是否有手指按下 */
volatile uint8_t isTouch = 0;
void scb12IntHandler(void *args)
{
    esp_rom_printf("1\n");
    int pin = (int)args;
    if (pin == SC12B_INT_IO)
    {
        isTouch = 1;
    }
}

/**
 * @description: 向触摸感应器的寄存器写值
 * @param {uint8_t} reg
 * @param {uint8_t} value
 * @return {*}
 */
static void Inf_SC12B_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t buff[2] = {reg, value};
    /* 参数1: i2c port 参数2: 7位地址(不要移位) 参数3: */
    i2c_master_write_to_device(0, SC12B_I2C_ADDR, buff, 2, 2000);
}

/**
 * @description: 从触摸感应器的寄存器读值
 * @param {uint8_t} reg
 * @return {*}
 */
static uint8_t Inf_SC12B_ReadReg(uint8_t reg)
{
    uint8_t data = 0;
    esp_err_t err = i2c_master_write_read_device(0,
                                                 SC12B_I2C_ADDR,
                                                 &reg,
                                                 1,
                                                 &data,
                                                 1,
                                                 2000);

    if (err != ESP_OK)
    {
        MY_LOGE("I2C read error: %s", esp_err_to_name(err));
    }
    else
    {
        MY_LOGE("I2C read reg 0x%02x: 0x%02x", reg, data);
    }

    return data;
}

/**
 * @description: 读取按键
 * @return {*} 读取到的按键
 */
static Touch_Key Inf_SC12B_ReadKey(void)
{
    /* 把读取到12个按键值存储到uint16的低12位 */
    uint8_t reg08 = Inf_SC12B_ReadReg(0x08);
    uint8_t reg09 = Inf_SC12B_ReadReg(0x09);
    uint16_t key = (reg08 << 4) | (reg09 >> 4);

    MY_LOGE("reg08 = 0x%02x, reg09 = 0x%02x, key = 0x%04x", reg08, reg09, key);

    Touch_Key touchKey = KEY_NO;
    /* 遍历低12位,找到被按下的按键 */
    for (size_t i = 0; i < 12; i++)
    {
        if (key >> i & 0x1)
        {
            switch (i)
            {
            case 0:
                touchKey = KEY_SHARP;
                break;
            case 1:
                touchKey = KEY_9;
                break;
            case 2:
                touchKey = KEY_8;
                break;
            case 3:
                touchKey = KEY_M;
                break;
            case 4:
                touchKey = KEY_4;
                break;
            case 5:
                touchKey = KEY_6;
                break;
            case 6:
                touchKey = KEY_5;
                break;
            case 7:
                touchKey = KEY_7;
                break;
            case 8:
                touchKey = KEY_3;
                break;
            case 9:
                touchKey = KEY_2;
                break;
            case 10:
                touchKey = KEY_1;
                break;
            case 11:
                touchKey = KEY_0;
                break;
            default:
                break;
            }
            MY_LOGE("Touch key: %d", touchKey);
            break;
        }
    }

    return touchKey;
}

/**
 * @description: 检测按键点击 (短按), 当手指离开,返回按下的键
 * @return {*}
 */
Touch_Key Inf_SC12B_KeyClick(void)
{
    Touch_Key key = KEY_NO;
    if (isTouch)
    {
        key = Inf_SC12B_ReadKey();
        isTouch = 0;
        // 添加短暂延时，防止按键抖动
        vTaskDelay(10);
    }
    return key;
}
