#ifndef __INF_WS2812_H__
#define __INF_WS2812_H__

#include <string.h>
#include <math.h>

#include "driver/rmt_tx.h"
#include "esp_task.h"
#include "Inf_SC12B.h"
#include "string.h"

void Inf_WS2812_Init(void);
void Inf_WS2812_LightKeyLed(Touch_Key key, uint8_t color[]);
void Inf_WS2812_LightAllKeyLeds(uint8_t color[]);

extern uint8_t black[3];
extern uint8_t white[3];
extern uint8_t red[3];
extern uint8_t green[3];
extern uint8_t blue[3];
extern uint8_t cyan[3];   /* 青色 */
extern uint8_t purple[3]; /* 紫色 */
#endif
