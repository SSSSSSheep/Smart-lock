/*
全彩led驱动:
  ESP32 驱动 WS2812（也称为 NeoPixel）LED 的常用方法是使用 RMT（Remote Control）外设。
  这种方法可以精确地生成 WS2812 所需的信号
*/
#include "Inf_WS2812.h"

/* 使用RMT 外设时的计时器分辨率，以赫兹（Hz）为单位 */
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
/* 使用的哪个gpio口 */
#define RMT_LED_STRIP_GPIO_NUM 6
/* led的数量 */
#define LED_NUMBERS 12

/*存储所有颜色数据(GRB) */
static uint8_t ledColors[36];

/* 定义几种常见颜色 */
uint8_t black[3] = {0, 0, 0};
uint8_t white[3] = {255, 255, 255};
uint8_t red[3] = {0, 255, 0};
uint8_t green[3] = {255, 0, 0};
uint8_t blue[3] = {0, 0, 255};
uint8_t cyan[3] = {255, 0, 255};   /* 青色 */
uint8_t purple[3] = {0, 255, 255}; /* 紫色 */

/* 声明rmt tx通道句柄 */
rmt_channel_handle_t led_chan = NULL;

/*声明一个编码器句柄 */
rmt_encoder_handle_t simple_encoder = NULL;

/* 0值 时序 */
static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,                                              /* 时序第一部分 高电平 */
    .duration0 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0H=0.3us
    .level1 = 0,                                              /* 时序第二部分 低电平*/
    .duration1 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0L=0.9us
};

/* 1值 时序 */
static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1H=0.9us
    .level1 = 0,
    .duration1 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1L=0.3us
};

/* 重置 */
static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 0,
    .duration0 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
    .level1 = 0,
    .duration1 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
};

/**
 * @description: 编码器回调函数
 * @param {void} *data 要发送的数据
 * @param {size_t} data_size 要发送的数据长度
 * @param {size_t} symbols_written 已经写入 RMT 缓冲区的符号数量。
 * @param {size_t} symbols_free RMT 缓冲区中剩余的可用符号数量。
 * @param {rmt_symbol_word_t} *symbols 指向 RMT 符号缓冲区的指针，用于存储编码后的 RMT 符号。
 * @param {bool} *done 指示编码是否完成的布尔标志。
 * @param {void} *arg 用户自定义的参数，用于在回调函数中传递额外信息。
 * @return {*}
 */
static size_t encoder_callback(const void *data,
                               size_t data_size,
                               size_t symbols_written,
                               size_t symbols_free,
                               rmt_symbol_word_t *symbols,
                               bool *done,
                               void *arg)
{
    // We need a minimum of 8 symbol spaces to encode a byte. We only
    // need one to encode a reset, but it's simpler to simply demand that
    // there are 8 symbol spaces free to write anything.
    if (symbols_free < 8)

    {
        return 0;
    }

    // We can calculate where in the data we are from the symbol pos.
    // Alternatively, we could use some counter referenced by the arg
    // parameter to keep track of this.
    size_t data_pos = symbols_written / 8;
    uint8_t *data_bytes = (uint8_t *)data;
    if (data_pos < data_size)

    {
        // Encode a byte
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1)

        {
            if (data_bytes[data_pos] & bitmask)

            {
                symbols[symbol_pos++] = ws2812_one;
            }
            else
            {
                symbols[symbol_pos++] = ws2812_zero;
            }
        }
        // We're done; we should have written 8 symbols.
        return symbol_pos;
    }
    else
    {
        // All bytes already are encoded.
        // Encode the reset, and we're done.
        symbols[0] = ws2812_reset;
        *done = 1;
        // Indicate end of the transaction.
        return 1;
        // we only wrote one symbol
    }
}
/**
 * @description: 初始化WS2812 全彩led
 * @return {*}
 */
void Inf_WS2812_Init(void)
{
    /* 1. 建立一个rmt tx通道 */
    /* 1.1 rmt tx通道配置 */
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    /* 1.2 创建通道 */
    rmt_new_tx_channel(&tx_chan_config, &led_chan);

    /* 2. 创建一个简单编码器 */
    /* 2.1 编码器配置 */
    const rmt_simple_encoder_config_t simple_encoder_cfg = {
        /* 编码器回调函数: 把用户通过rmt_transmit 传递数据,编码成硬件要发送的符号*/
        .callback = encoder_callback
        // Note we don't set min_chunk_size here as the default of 64 is good enough.
    };
    /* 2.2 创建编码器 */
    rmt_new_simple_encoder(&simple_encoder_cfg, &simple_encoder);

    /* 3. 使能通道 */
    rmt_enable(led_chan);

    /* 4. 点亮所有灯,然后关闭 */
    Inf_WS2812_LightAllKeyLeds(white);
    vTaskDelay(500);
    Inf_WS2812_LightAllKeyLeds(black);
}

/**
 * @description: 点亮led
 * @return {*}
 */
static void Inf_WS2812_LightLeds(void)
{
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };
    /* 1. 发送数据 */
    rmt_transmit(led_chan,          /* 通道 */
                 simple_encoder,    /* 编码器 */
                 ledColors,         /* 要发送的颜色数据 */
                 sizeof(ledColors), /* 要发送的数据缓冲区的长度 */
                 &tx_config);
    /* 发送配置 */

    /* 2. 等待发送完成 */
    rmt_tx_wait_all_done(led_chan, portMAX_DELAY);
}

/**
 * @description: 点亮指定的按键对应的led为指定的颜色
 * @param {Touch_Key} key 指定的按键
 * @param {uint8_t} color 指定的颜色 长度必须是3 [G, R, B]
 * @return {*}
 */
void Inf_WS2812_LightKeyLed(Touch_Key key, uint8_t color[])
{
    memset(ledColors, 0, sizeof(ledColors));
    uint8_t index = key;
    memcpy(&ledColors[index * 3], color, 3);
    Inf_WS2812_LightLeds();
}

/**
 * @description: 点亮所有led为相同的指定的颜色
 * @param {uint8_t} color 指定的颜色 长度必须是3 [G, R, B]
 * @return {*}
 */
void Inf_WS2812_LightAllKeyLeds(uint8_t color[])
{
    for (uint8_t i = 0; i < sizeof(ledColors); i += 3)

    {
        memcpy(&ledColors[i], color, 3);
    }
    Inf_WS2812_LightLeds();
}