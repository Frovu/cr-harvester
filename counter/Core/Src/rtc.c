#include "counter.h"
#define CENTURY 2000

RTC_TimeTypeDef rtcTime = { 0 };
RTC_DateTypeDef rtcDate = { 0 };
HAL_StatusTypeDef status;
extern RTC_HandleTypeDef hrtc;

HAL_StatusTypeDef RTC_WriteDateTime(DateTime *dt, uint32_t timeout) {
    second_counter = dt->tm_sec;
    rtcTime.Hours   = dt->tm_hour;
    rtcTime.Minutes = dt->tm_min;
    rtcTime.Seconds = dt->tm_sec;
    rtcDate.Date    = dt->tm_mday;
    rtcDate.Month   = dt->tm_mon;
    rtcDate.Year    = dt->tm_year % 100;
    status = HAL_RTC_SetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN);
    if (status != HAL_OK) return status;
    status = HAL_RTC_SetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN);
    return status;
}


HAL_StatusTypeDef RTC_ReadDateTime (DateTime *dt, uint32_t timeout) {
    status = HAL_RTC_GetTime(&hrtc, &rtcTime, RTC_FORMAT_BIN);
    if (status != HAL_OK) return status;
    status = HAL_RTC_GetDate(&hrtc, &rtcDate, RTC_FORMAT_BIN);
    if (status != HAL_OK) return status;
    dt->tm_hour = rtcTime.Hours;
    dt->tm_min  = rtcTime.Minutes;
    dt->tm_sec  = rtcTime.Seconds;
    dt->tm_mday = rtcDate.Date;
    dt->tm_mon  = rtcDate.Month;
    dt->tm_year = rtcDate.Year + CENTURY;
    return HAL_OK;
}
