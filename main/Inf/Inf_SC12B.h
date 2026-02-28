#ifndef __INF_SC12B_H__
#define __INF_SC12B_H__

#include "Com_Debug.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_intr_types.h"

// 定义引脚的宏
#define I2C_SC12B_SDA GPIO_NUM_2
#define I2C_SC12B_SCL GPIO_NUM_1
#define I2C_SC12B_INTR GPIO_NUM_0

// 定义I2C工作时钟频率
#define I2C_SC12B_FREQ_HZ 400000

// 定义SC12B设备地址
#define I2C_SC12B_ADDR (0x42)

typedef enum
{
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_SHARP,
    KEY_M,
    KEY_NO
} Touch_Key;

/**
 * @brief 初始化I2C模块
 */
void Inf_SC12B_Init(void);

/**
 * @brief 向SC12B写入寄存器
 * @param reg_addr 寄存器地址
 * @param data 要写入的数据
 */
void Inf_SC12B_WriteReg(uint8_t reg_addr, uint8_t data);

/**
 * @brief 从SC12B读取寄存器
 */
uint8_t Inf_SC12B_ReadReg(uint8_t reg_addr);

/**
 * @brief 从SC12B读取按键
 * @return Touch_Key 按键值
 */
Touch_Key Inf_SC12B_ReadKey(void);

/**
 * @brief 根据中断标记位 获取SC12B按键点击事件
 * @return Touch_Key 按键值
 */
Touch_Key Inf_SC12B_GetKeyClick(void);

#endif /* __INF_SC12B_H__ */
