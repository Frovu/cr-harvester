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

#define FLAG_EVENT_BASE 0x1
#define FLAG_EVENT_DATA 0x2
#define FLAG_BMP_OK 0x10

#define COUNTER_DATA_RATE 60

// Called continuously from core while loop
void event_loop();
// Core events handlers
void data_collection_event();
void base_clock_event();
// Called every second from system RTC IRQ handler
void base_clock_interrupt_handler();


#endif
