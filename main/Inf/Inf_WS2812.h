#ifndef __INF_WS2812_H
#define __INF_WS2812_H

#include <string.h>
#include <math.h>

#include "driver/rmt_tx.h"
#include "esp_task.h"
#include "Inf_SC12B.h"
#include "string.h"

extern uint8_t black[3];
extern uint8_t white[3];
extern uint8_t red[3];
extern uint8_t green[3];
extern uint8_t blue[3];
extern uint8_t cyan[3];   /* 青色 */
extern uint8_t purple[3]; /* 紫色 */

/**
 * @description: 初始化WS2812 全彩led
 */
void Inf_WS2812_Init(void);

/**
 * @description: 点亮指定的按键led为指定的颜色
 * @param {Touch_Key} key 指定的按键
 * @param {uint8_t} color 指定的颜色 长度必须是3 [G, R, B]
 */
void Inf_WS2812_LightKeyLed(Touch_Key key, uint8_t color[]);

/**
 * @description: 点亮所有led为相同的指定的颜色
 * @param {uint8_t} color 指定的颜色 长度必须是3 [G, R, B]
 */
void Inf_WS2812_LightAllKeyLeds(uint8_t color[]);

/**
 * @description: 点亮led回显为黑色
 */
void Inf_WS2812_LightLedBlack(void);

#endif