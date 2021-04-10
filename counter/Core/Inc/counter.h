/**
  ******************************************************************************
  * @file           : counter.c
  * @brief          : Header file for core counter logic
  ******************************************************************************
*/

#ifndef __COUNTER_H
#define __COUNTER_H

#include "main.h"
#include "bmp280.h"
#include "ds3231.h"
#include "at25df321.h"
#include "internet.h"
#include "config_server.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define IS_SET(f)  (flags & f)
#define NOT_SET(f) ((flags & f) == 0)
#define RAISE(f)   (flags |= f)
#define TOGGLE(f)  (flags ^= f)

#define FLAG_EVENT_BASE     0x01
#define FLAG_RTC_ALARM      0x02
#define FLAG_DATA_SENDING   0x04
#define FLAG_TIME_TRUSTED   0x08

#define FLAG_BMP_OK         0x10
#define FLAG_FLASH_OK       0x20
#define FLAG_RTC_OK         0x40
#define FLAG_W5500_OK       0x80

#define FLAG_DHCP_RUN       0x100
#define FLAG_NTP_SYNC       0x200
#define FLAG_DNS_RUN        0x400
#define FLAG_FLASH_INIT     0x800

#define FLAGS_INITIAL (FLAG_RTC_ALARM | FLAG_NTP_SYNC)

#define FLAG_OK_MASK        0xF0
#define FLAG_OK_SHIFT       0x04

typedef enum {
  DEV_RTC,
  DEV_BMP,
  DEV_FLASH,
  DEV_W5500
} device_t;

// #define RTC_SHORT_CYCLE // switch from 60s to 1s cycle (ONLY FOR DEBUG)
#ifdef RTC_SHORT_CYCLE
  #define RTC_ALARM_REG RTC_REG_ALARM1
  #define RTC_ALARM_ENABLE_BIT RTC_CONTROL_A1IE
  #define RTC_ALARM_CONFIG { 0x80, 0x80, 0x80, 0x80 } // set 4 mask bits (alarm1 every second)
#else
  #define RTC_ALARM_REG RTC_REG_ALARM2
  #define RTC_ALARM_ENABLE_BIT RTC_CONTROL_A2IE
  #define RTC_ALARM_CONFIG { 0x80, 0x80, 0x80 } // set 3 mask bits (alarm2 every minute)
#endif
#define RTC_CONFIG (RTC_ALARM_ENABLE_BIT | RTC_CONTROL_INTCN)

#define DEFAULT_TIMEOUT            300
#ifdef RTC_SHORT_CYCLE
  #define SENDING_TIMEOUT          100
  #define BASE_PERIOD_LEN_MS       1000
#else
  #define SENDING_TIMEOUT          3000
  #define BASE_PERIOD_LEN_MS       60000
#endif
#define PROBLEM_FIXING_PERIOD      3000
#define FLASH_INIT_TIME            30000

#define NTP_SYNC_PERIOD             30 // in cycles (minutes)
#define TIME_TRUST_PERIOD          720 // how long is time accounted as trusted after ntp sync

#define BASE_EVENT_WATCHDOG_MS   (BASE_PERIOD_LEN_MS + 2000)
#define CHANNELS_COUNT           12
// if and only if more than DATA_BUFFER_LEN lines fail to send external flash memory is used
#define DATA_BUFFER_LEN          8



#define GPIO_RTC_IRQ    GPIO_PIN_1

#define GPIO_CH_0       GPIO_PIN_12
#define GPIO_CH_1       GPIO_PIN_13
#define GPIO_CH_2       GPIO_PIN_14
#define GPIO_CH_3       GPIO_PIN_15
#define GPIO_CH_4       GPIO_PIN_8
#define GPIO_CH_5       GPIO_PIN_9
#define GPIO_CH_6       GPIO_PIN_10
#define GPIO_CH_7       GPIO_PIN_11
#define GPIO_CH_8       GPIO_PIN_4
#define GPIO_CH_9       GPIO_PIN_5
#define GPIO_CH_10      GPIO_PIN_6
#define GPIO_CH_11      GPIO_PIN_7

static const uint8_t GPIO_LOOKUP_CHANNEL[16] = {
  -1, -1, -1, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3, -1
};

typedef struct {
  uint32_t timestamp;
  uint32_t cycle;
  uint32_t info;
  float temperature;
  float pressure;
  uint16_t counts[CHANNELS_COUNT];
} DataLine;

/********************* System Core **********************/
// Initialization routines
uint8_t try_init_rtc();
uint8_t try_init_bmp();
uint8_t try_init_flash();
void counter_init();
// Called continuously from core while loop
void event_loop();
// Core events handlers
void base_periodic_event();
/********************* Data Handling **********************/
void init_read_flash();
void data_period_transition(const volatile uint16_t * counts, DateTime *dt, float t, float p);
int32_t data_send_one(uint32_t timeout);
/********************* Data Sending **********************/


#endif
