/*
*  A simple HAL driver for DS3231 Real Time Clock
*     Author: Frovy
*/

#ifndef __DS3231_H
#define __DS3231_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <time.h>

#define RTC_DEFAULT_ADDR    0xD0

#define RTC_REG_DATE        0x00
#define RTC_REG_ALARM1      0x07
#define RTC_REG_ALARM2      0x0B
#define RTC_REG_CONTROL     0x0E
#define RTC_REG_STATUS      0x0F

#define RTC_CONTROL_A1IE    0x01
#define RTC_CONTROL_A2IE    0x02
#define RTC_CONTROL_INTCN   0x04
#define RTC_CONTROL_SQW_1HZ 0x00

typedef struct tm DateTime;

#define FROM_BCD(val)   ((val >> 4) * 10 + (val & 0x0F))
#define TO_BCD(val)     (((val / 10) << 4) | (val % 10))

HAL_StatusTypeDef RTC_init(I2C_HandleTypeDef *i2ch, uint16_t address, uint8_t config, uint32_t timeout);

HAL_StatusTypeDef RTC_ConfigAlarm(uint16_t addr, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef RTC_ClearAlarm(uint32_t timeout);
// alarm functionality could be implemented here, but not today

HAL_StatusTypeDef RTC_WriteDateTime(DateTime *dt, uint32_t timeout);
HAL_StatusTypeDef RTC_ReadDateTime (DateTime *dt, uint32_t timeout);

#endif
