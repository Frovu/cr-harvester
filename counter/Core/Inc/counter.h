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

#define FLAG_EVENT_BASE     0x01
#define FLAG_EVENT_DATA     0x02
#define FLAG_BMP_OK         0x10
#define FLAG_FLASH_OK       0x20

#define DEFAULT_TIMEOUT      300
#define COUNTER_DATA_RATE     60
#define CHANNELS_COUNT        12

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
  uint16_t counts[CHANNELS_COUNT];
  float temperature;
  float pressure;
} DataLine;

// Called continuously from core while loop
void event_loop();
// Core events handlers
void data_collection_event();
void base_clock_event();
// Called every second from system RTC IRQ handler
void base_clock_interrupt_handler();


#endif
