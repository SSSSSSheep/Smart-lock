#ifndef __INF_BDR6120_H__
#define __INF_BDR6120_H__

#include "driver/gpio.h"
#include "esp_task.h"
void Inf_BDR6120_Init(void);

void Inf_BDR6120_ForwardRotation(void);

void Inf_BDR6120_ReverseRotation();

void Inf_BDR6120_Brake();

void Inf_BDR6120_LockOpen(void);

#endif
