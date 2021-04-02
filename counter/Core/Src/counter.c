/**
  ******************************************************************************
  * @file           : counter.c
  * @brief          : Core counter logic
  ******************************************************************************
*/
#include "counter.h"

uint16_t flags = 0;
uint16_t seconds_counter = 0;
uint32_t cycle_counter = 0;


void event_loop() {
  if (flags & FLAG_EVENT_DATA) {
    data_collection_event();
    flags ^= FLAG_EVENT_DATA;
  }
  if (flags & FLAG_EVENT_BASE) {
    base_clock_event();
    flags ^= FLAG_EVENT_BASE;
  }
}

void data_collection_event() {

}

void base_clock_event() {
  // blink onboard led to show that we are alive
HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_RESET);
HAL_Delay(5);
HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);
//  HAL_GPIO_TogglePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin);

}

void base_clock_interrupt_handler() {
  ++seconds_counter;
  flags |= FLAG_EVENT_BASE;
  if(seconds_counter >= COUNTER_DATA_RATE) {
    flags |= FLAG_EVENT_DATA;
    seconds_counter = 0;
    ++cycle_counter;
  }
}
