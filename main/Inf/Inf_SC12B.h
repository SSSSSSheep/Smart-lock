#ifndef __INF_SC12B_H__
#define __INF_SC12B_H__

#include "stdio.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "Com_Debug.h"

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

void Inf_SC12B_Init(void);

Touch_Key Inf_SC12B_KeyClick(void);
#endif
