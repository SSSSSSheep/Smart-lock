#include "Inf_SC12B.h"

// 定义一个标记位
uint8_t isTouch = 0;
/**
 * @brief SC12B按键中断处理函数
 * @param args 中断引脚号
 */
void Inf_SC12B_IntrHandler(void *args)
{
    // esp_rom_printf("Inf_SC12B_IntrHandler\n");
    isTouch = 1;
}

void Inf_SC12B_Init(void)
{
    // 1. 准备配置信息
    i2c_config_t config = {};
    config.mode = I2C_MODE_MASTER;
    config.sda_io_num = I2C_SC12B_SDA;
    config.scl_io_num = I2C_SC12B_SCL;
    config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    config.master.clk_speed = I2C_SC12B_FREQ_HZ;

    // 2. 让配置信息生效
    i2c_param_config(I2C_NUM_0, &config);
    // 3. 安装I2C驱动，自动I2C外设模块
    i2c_driver_install(I2C_NUM_0, config.mode, 0, 0, 0);
    // 4. 调整灵敏度
    Inf_SC12B_WriteReg(0x00, 0x04);
    Inf_SC12B_WriteReg(0x01, 0x04);
    // 5. GPIO引脚中断
    // 5.1 中断引脚配置信息并让其生效
    gpio_config_t io_config = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << I2C_SC12B_INTR),
        .pull_down_en = 1,
        .pull_up_en = 0,
    };
    gpio_config(&io_config);

    // 5.2 安装中断服务 采用
    gpio_install_isr_service(0);
    // 5.3 注册中断回调函数
    gpio_isr_handler_add(I2C_SC12B_INTR,
                         Inf_SC12B_IntrHandler,
                         (void *)I2C_SC12B_INTR);
}

void Inf_SC12B_WriteReg(uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};

    i2c_master_write_to_device(I2C_NUM_0,
                               I2C_SC12B_ADDR,
                               write_buf,
                               sizeof(write_buf),
                               2000);
}

uint8_t Inf_SC12B_ReadReg(uint8_t reg_addr)
{
    uint8_t data = 0;
    i2c_master_write_read_device(I2C_NUM_0,
                                 I2C_SC12B_ADDR,
                                 &reg_addr,
                                 1,
                                 &data,
                                 1,
                                 2000);
    return data;
}

Touch_Key Inf_SC12B_ReadKey(void)
{
    // 1. 读取08H与09H寄存器
    uint16_t regData = ((Inf_SC12B_ReadReg(0x08) << 8) | (Inf_SC12B_ReadReg(0x09)));
    // 2. 根据读取到的寄存器值，判断是哪一个按键被按下
    Touch_Key touchKey = KEY_NO;
    switch (regData)
    {
    case 0x8000:
        touchKey = KEY_0;
        break;
    case 0x4000:
        touchKey = KEY_1;
        break;
    case 0x2000:
        touchKey = KEY_2;
        break;
    case 0x1000:
        touchKey = KEY_3;
        break;
    case 0x0800:
        touchKey = KEY_7;
        break;
    case 0x0400:
        touchKey = KEY_5;
        break;
    case 0x0200:
        touchKey = KEY_6;
        break;
    case 0x0100:
        touchKey = KEY_4;
        break;
    case 0x0080:
        touchKey = KEY_M;
        break;
    case 0x0040:
        touchKey = KEY_8;
        break;
    case 0x0020:
        touchKey = KEY_9;
        break;
    case 0x0010:
        touchKey = KEY_SHARP;
        break;
    default:
        break;
    }
    return touchKey;
}
