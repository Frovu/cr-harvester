/*
*  A simple HAL driver for DS3231 Real Time Clock
*     Author: Frovy
*/
#include "ds3231.h"

I2C_HandleTypeDef *rtc_i2ch;
uint16_t dev_addr;

HAL_StatusTypeDef RTC_init(I2C_HandleTypeDef *i2ch, uint16_t address, uint8_t config, uint32_t timeout)
{
  rtc_i2ch = i2ch;
  dev_addr = address;
  return HAL_I2C_Mem_Write(rtc_i2ch, dev_addr, RTC_REG_CONTROL, 1, &config, 1, timeout);
}

HAL_StatusTypeDef RTC_ConfigAlarm(uint16_t addr, const uint8_t *data, uint32_t timeout)
{
  uint16_t size = (addr == RTC_REG_ALARM1) ? 0x4 : 0x3;
  return HAL_I2C_Mem_Write(rtc_i2ch, dev_addr, addr, 1, data, size, timeout);
}

HAL_StatusTypeDef RTC_ClearAlarm(uint32_t timeout)
{
  uint8_t reg = 0u;
  return HAL_I2C_Mem_Write(rtc_i2ch, dev_addr, RTC_REG_STATUS, 1, &reg, 1, timeout);
}

HAL_StatusTypeDef RTC_WriteDateTime(DateTime *dt, uint32_t timeout)
{
  uint8_t buf[7];
  buf[0] = TO_BCD(dt->tm_sec);
  buf[1] = TO_BCD(dt->tm_min);
  buf[2] = TO_BCD(dt->tm_hour);
  // buf[3] = TO_BCD(dt->tm_wday);   FIXME: uncomment if weekday isn't useless
  buf[4] = TO_BCD(dt->tm_mday);
  buf[5] = TO_BCD(dt->tm_mon);
  buf[6] = TO_BCD((dt->tm_year - RTC_CENTURY_START));
  return HAL_I2C_Mem_Write(rtc_i2ch, dev_addr, RTC_REG_DATE, 1, buf, 7, timeout);
}

HAL_StatusTypeDef RTC_ReadDateTime(DateTime *dt, uint32_t timeout)
{
  uint8_t buf[7];
  HAL_StatusTypeDef status = HAL_I2C_Mem_Read(rtc_i2ch, dev_addr, RTC_REG_DATE, 1, buf, 7, timeout);
  if (status == HAL_OK)
  {
    dt->tm_sec  = FROM_BCD(buf[0]);
    dt->tm_min  = FROM_BCD(buf[1]);
    dt->tm_hour = FROM_BCD(buf[2]);
    // dt->tm_wday = FROM_BCD(buf[3]); FIXME: uncomment if weekday isn't useless
    dt->tm_mday = FROM_BCD(buf[4]);
    dt->tm_mon  = FROM_BCD(buf[5]);
    dt->tm_year = RTC_CENTURY_START + FROM_BCD(buf[6]);
    dt->tm_isdst = 0;
  }
  return status;
}
