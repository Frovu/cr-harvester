/**
  ******************************************************************************
  * @file           : counter.c
  * @brief          : Core counter logic
  ******************************************************************************
*/
#include "counter.h"

extern I2C_HandleTypeDef hi2c2;
extern SPI_HandleTypeDef hspi1;
BMP280_HandleTypedef bmp280;

volatile uint16_t saved_counts[CHANNELS_COUNT];
volatile uint16_t counters[CHANNELS_COUNT];

volatile uint16_t flags = FLAG_RTC_ALARM | FLAG_SKIP_PERIOD;
uint32_t cycle_counter = 0;
uint32_t last_alarm_reset = 0;

uint8_t try_init_bmp() {
  if (flags & FLAG_BMP_OK)
    return 1;
  if (bmp280_init(&bmp280, &bmp280.params)) {
    flags |= FLAG_BMP_OK;
    debug_printf("BMP280 init success, id = 0x%x\r\n", bmp280.id);
    return 1;
  } else {
    debug_printf("BMP280 init failed\r\n");
    return 0;
  }
}

uint8_t try_init_flash() {
  if (flags & FLAG_FLASH_OK)
    return 1;
  if (at25_is_valid() && at25_is_ready()) {
    at25_global_unprotect();
    flags |= FLAG_FLASH_OK;
    debug_printf("AT25DF321 init success\r\n");
    return 1;
  } else {
    debug_printf("AT25DF321 invalid\r\n");
    return 0;
  }
}

uint8_t try_init_rtc() {
  if (RTC_init(&hi2c2, RTC_DEFAULT_ADDR, RTC_CONTROL_A2IE|RTC_CONTROL_INTCN, DEFAULT_TIMEOUT)) {
    debug_printf("RTC init success\r\n");
    return 1;
  } else {
    debug_printf("RTC init failed\r\n");
    return 0;
  }
}

void counter_init() {
  debug_printf("INIT\r\n");
  // ******************** BMP280 ********************
  bmp280_init_default_params(&bmp280.params);
  bmp280.addr = BMP280_I2C_ADDRESS_0;
  bmp280.i2c = &hi2c2;
  for (int i=0; !try_init_bmp() && i < 3; ++i) {
    HAL_Delay(500);
  }
  // ******************* AT25DF321 ******************
  at25_init(&hspi1, AT25_CS_GPIO_Port, AT25_CS_Pin);
  for (int i=0; !try_init_flash() && i < 3; ++i) {
    HAL_Delay(300);
  }
  // ******************** DS3231 ********************
  while (!try_init_rtc()) {
    HAL_Delay(300);
  }
}

void event_loop() {
  if (flags & FLAG_EVENT_BASE) {
    base_clock_event();
    flags ^= FLAG_EVENT_BASE;
  }
  if (flags & FLAG_RTC_ALARM) {
    if (RTC_ClearAlarm(50) == HAL_OK) {
      last_alarm_reset = HAL_GetTick();
      flags ^= FLAG_RTC_ALARM;
    }
  }
  else
  {
    /* If RTC does not produce alarm IRQ for more than a minute,
    *  it means something is wrong with RTC and the only thing we
    *  can do here is just to try to re-initialize RTC's alarm registers
    *  with some period and hope it will be restored in its place
    */
    if (HAL_GetTick() - last_alarm_reset > BASE_EVENT_WATCHDOG_MS)
    {
      debug_printf("period watchdog triggered\r\n");
      if (try_init_rtc() != HAL_OK) {
        // error led on
        HAL_Delay(2500);
      } else {
        debug_printf("period watchdog reset!\r\n");
        // error led off
      }
    }
  }

}
void base_periodic_event() {
  float t, p;
  if (flags & FLAG_BMP_OK) {
    for (int i=0; !bmp280_read_float(&bmp280, &t, &p, NULL) && (i < 3); ++i) {
      debug_printf("BMP280 readout failed\r\n");
      HAL_Delay(100);
    }
    bmp280_force_measurement(&bmp280);
  }

  debug_printf("%.2f hPa / %.2f C\r\n", p/100, t);
  // blink onboard led to show that we are alive
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_RESET);
  HAL_Delay(3);
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);

  DateTime dt;
  RTC_ReadDateTime(&dt, DEFAULT_TIMEOUT);
  char buf[32];
  strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &dt);
  debug_printf("time: %s\r\n", buf);

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  if (GPIO_Pin == GPIO_RTC_IRQ)
  {
    if ((flags & FLAG_RTC_ALARM) == 0)
    {
      flags |= FLAG_RTC_ALARM;
      flags |= FLAG_EVENT_BASE;
      for (uint8_t i=0; i<CHANNELS_COUNT; ++i) {
        saved_counts[i] = counters[i];
      }
      ++cycle_counter;
    }
  }
  else
  {
    uint8_t idx = 0;
    while (GPIO_Pin >>= 1) {
      ++idx;
    }
    idx = GPIO_LOOKUP_CHANNEL[idx];
    if (idx < CHANNELS_COUNT) {
      ++counters[idx];
    }
  }
}
