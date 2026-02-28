#ifndef __INF_WTN6170_H__
#define __INF_WTN6170_h__

#include "driver/gpio.h"
#include "sys/unistd.h"
#include "esp_task.h"

#define WTN6170_DATA_PIN GPIO_NUM_9
#define WTN6170_DATA_L (gpio_set_level(WTN6170_DATA_PIN, 0))
#define WTN6170_DATA_H (gpio_set_level(WTN6170_DATA_PIN, 1))

#define sayWithoutInt() Inf_WTN6170_SendCmd(0xf3)

#define sayNum(x) Inf_WTN6170_SendCmd(x + 1)
#define sayWaterDrop() Inf_WTN6170_SendCmd(11)
#define sayBuzzer() Inf_WTN6170_SendCmd(12)
#define sayAlarm() Inf_WTN6170_SendCmd(13)
#define sayDoorBell() Inf_WTN6170_SendCmd(14)
#define sayFail() Inf_WTN6170_SendCmd(16)
#define sayPassword() Inf_WTN6170_SendCmd(19)
#define sayDoorOpen() Inf_WTN6170_SendCmd(25)
#define sayDoorClose() Inf_WTN6170_SendCmd(26)
#define sayIllegalOperation() Inf_WTN6170_SendCmd(28)
#define sayVerify() Inf_WTN6170_SendCmd(31)
#define sayFinish() Inf_WTN6170_SendCmd(33)
#define sayPressSharp() Inf_WTN6170_SendCmd(34)
#define sayDelSucc() Inf_WTN6170_SendCmd(36)
#define sayDelFail() Inf_WTN6170_SendCmd(37)
#define sayDelAll() Inf_WTN6170_SendCmd(39)
#define sayVerifySucc() Inf_WTN6170_SendCmd(41)
#define sayVerifyFail() Inf_WTN6170_SendCmd(42)
#define saySetSucc() Inf_WTN6170_SendCmd(43)
#define saySetFail() Inf_WTN6170_SendCmd(44)
#define sayOperateSucc() Inf_WTN6170_SendCmd(46)
#define sayOperateFail() Inf_WTN6170_SendCmd(47)
#define sayInvalid() Inf_WTN6170_SendCmd(48)
#define sayAddSucc() Inf_WTN6170_SendCmd(49)
#define sayAddFail() Inf_WTN6170_SendCmd(50)
#define sayAddUser() Inf_WTN6170_SendCmd(51)
#define sayDelUser() Inf_WTN6170_SendCmd(52)
#define sayUserFull() Inf_WTN6170_SendCmd(53)
#define sayAddAdmin() Inf_WTN6170_SendCmd(54)
#define sayDelAdmin() Inf_WTN6170_SendCmd(55)
#define sayAdminFull() Inf_WTN6170_SendCmd(56)
#define sayRetry() Inf_WTN6170_SendCmd(58)
#define sayInputUserPassword() Inf_WTN6170_SendCmd(64)
#define sayInputUserPasswordAgain() Inf_WTN6170_SendCmd(65)
#define sayPasswordAddSucc() Inf_WTN6170_SendCmd(66)
#define sayPasswordAddFail() Inf_WTN6170_SendCmd(67)
#define sayPasswordVerifySucc() Inf_WTN6170_SendCmd(68)
#define sayPasswordVerifyFail() Inf_WTN6170_SendCmd(69)
#define sayInputAdminPassword() Inf_WTN6170_SendCmd(70)
#define sayInputAdminPasswordAgain() Inf_WTN6170_SendCmd(71)
#define sayAddUserFingerprint() Inf_WTN6170_SendCmd(76)
#define sayDelUserFingerprint() Inf_WTN6170_SendCmd(77)
#define sayPlaceFinger() Inf_WTN6170_SendCmd(80)
#define sayPlaceFingerAgain() Inf_WTN6170_SendCmd(81)
#define sayTakeAwayFinger() Inf_WTN6170_SendCmd(82)
#define sayFingerprintAddSucc() Inf_WTN6170_SendCmd(83)
#define sayFingerprintAddFail() Inf_WTN6170_SendCmd(84)
#define sayFingerprintVerifySucc() Inf_WTN6170_SendCmd(85)
#define sayFingerprintVerifyFail() Inf_WTN6170_SendCmd(86)

// 语音模块初始化
void Inf_WTN6170_Init(void);

/**
 * @description: 发送指令
 * @param: {uint8_t} cmd指令
 */
void Inf_WTN6170_SendCmd(uint8_t cmd);

#endif // !__INF_WTN6170_H__
