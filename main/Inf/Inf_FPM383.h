#ifndef __INF_FPM383_H__
#define __INF_FPM383_H__

#include "driver/uart.h"
#include "stdio.h"
#include "driver/gpio.h"
#include "esp_task.h"
#include "Com_Config.h"
#include "string.h"
#include "Inf_WTN6170.h"
#include "Com_Debug.h"

#define LED_RED_FLICKER Inf_FPM383_LedControl(2, 0x04, 0x04, 0);
#define LED_GREEN_FLICKER Inf_FPM383_LedControl(2, 0x02, 0x02, 0);
#define LED_BLUE_FLICKER Inf_FPM383_LedControl(2, 0x01, 0x01, 0);

#define LED_RED_ON Inf_FPM383_LedControl(3, 0x04, 0x04, 0);
#define LED_GREEN_ON Inf_FPM383_LedControl(3, 0x02, 0x02, 0);
#define LED_BLUE_ON Inf_FPM383_LedControl(3, 0x01, 0x01, 0);

#define LED_COLOR_OFF Inf_FPM383_LedControl(4, 0x07, 0x07, 0);

void Inf_FPM383_Init(void);

Com_Status Inf_FPM383_ReadData(uint16_t recvLen, uint32_t timeout);

void Inf_FPM383_ReadId(void);

void Inf_FPM383_LedControl(uint8_t fun, uint8_t startColor, uint8_t endColor, uint8_t cycle);

void Inf_FPM383_CanceAutoAction(void);

void Inf_FPM383_StepEnroll(uint16_t pageId);

void Inf_FPM383_AutoEnroll(uint16_t id);

Com_Status Inf_FPM383_AutoIdentify(void);

void Inf_FPM383_Sleep(void);

void Inf_FPM383_ClearAll(void);

uint16_t Inf_FPM383_GetValidTempleteNum(void);

uint8_t Inf_FPM383_GetMinAviableId(void);

Com_Status Inf_FPM383_getImage(void);

int16_t Inf_FPM383_SearchTemplete(void);

Com_Status Inf_FPM383_DeleteTemplete(uint16_t pageId);

void Inf_FPM383_DeleteFingerPrint(void);

void Inf_FPM383_CheckSum(uint8_t buff[], uint8_t buffLen);

void printRecv(char *pre, uint8_t len);

#endif
