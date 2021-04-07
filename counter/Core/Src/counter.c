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

volatile uint16_t flags = FLAG_RTC_ALARM;

volatile uint16_t saved_counts[CHANNELS_COUNT];
volatile uint16_t counters[CHANNELS_COUNT];

uint32_t cycle_counter = 0;
uint32_t last_period_tick = 0;

uint8_t try_init_bmp() {
  if (IS_SET(FLAG_BMP_OK))
    return 1;
  if (bmp280_init(&bmp280, &bmp280.params)) {
    RAISE(FLAG_BMP_OK);
    debug_printf("BMP280 init success, id = 0x%x\r\n", bmp280.id);
    return 1;
  } else {
    debug_printf("BMP280 init failed\r\n");
    return 0;
  }
}

uint8_t try_init_flash() {
  if (IS_SET(FLAG_FLASH_OK))
    return 1;
  if (at25_is_valid() && at25_is_ready()) {
    at25_global_unprotect();
    init_read_flash();
    RAISE(FLAG_FLASH_OK);
    debug_printf("AT25DF321 init success\r\n");
    return 1;
  } else {
    debug_printf("AT25DF321 invalid\r\n");
    return 0;
  }
}

uint8_t try_init_rtc() {
  if (RTC_init(&hi2c2, RTC_DEFAULT_ADDR, RTC_CONFIG, DEFAULT_TIMEOUT) == HAL_OK) {
    uint8_t alarm_data[] = RTC_ALARM_CONFIG;
    if (RTC_ConfigAlarm(RTC_REG_ALARM2, alarm_data, DEFAULT_TIMEOUT) == HAL_OK) {
      if (RTC_ClearAlarm(DEFAULT_TIMEOUT) == HAL_OK) {
        debug_printf("RTC init success\r\n");
        return 1;
      }
    }
  }
  debug_printf("RTC init failed!\r\n");
  return 0;

}

void counter_init()
{
  debug_printf("\r\n\r\nINIT\r\n");
  // ******************** BMP280 ********************
  bmp280_init_default_params(&bmp280.params);
  bmp280.addr = BMP280_I2C_ADDRESS_0;
  bmp280.i2c = &hi2c2;
  for (int i=0; !try_init_bmp() && i < 3; ++i) {
    HAL_Delay(500);
  }
  // ******************** DS3231 ********************
  while (!try_init_rtc()) {
    HAL_Delay(500);
  }
  // ******************* AT25DF321 ******************
  at25_init(&hspi1, AT25_CS_GPIO_Port, AT25_CS_Pin);
  for (int i=0; !try_init_flash() && i < 3; ++i) {
    HAL_Delay(300);
  }
}

void event_loop() {
  if (IS_SET(FLAG_EVENT_BASE))
  {
    last_period_tick = HAL_GetTick();
    base_periodic_event();
    TOGGLE(FLAG_EVENT_BASE);
  }
  if (IS_SET(FLAG_RTC_ALARM))
  {
    if (RTC_ClearAlarm(50) == HAL_OK) {
      TOGGLE(FLAG_RTC_ALARM);
    }
  }
  else
  {
    /* If RTC does not produce alarm IRQ for more than a minute,
    *  it means something is wrong with RTC and the only thing we
    *  can do here is just to try to re-initialize RTC's alarm registers
    *  with some period and hope it will be restored in its place
    */
    if (HAL_GetTick() - last_period_tick > BASE_EVENT_WATCHDOG_MS)
    {
      debug_printf("period watchdog triggered\r\n");
      if (try_init_rtc() != HAL_OK) {
        // error led on
        HAL_Delay(1000);
      } else {
        debug_printf("period watchdog reset!\r\n");
        // error led off
      }
    }
  }
  int32_t time_left = BASE_PERIOD_LEN_MS - HAL_GetTick() + last_period_tick;
  if (time_left > SENDING_TIMEOUT * 2)
  {
    if (IS_SET(FLAG_DATA_SENDING)) {
    	int32_t storage_stat = data_send_one(SENDING_TIMEOUT);
      if (storage_stat == 0) {
        // everything sent
        TOGGLE(FLAG_DATA_SENDING);
      } else if (storage_stat < 0) {
        // if failed to send line wait until something probably fixes idk
        HAL_Delay(1000); // FIXME: probably too much
      }
    }
    if (NOT_SET(FLAG_BMP_OK)) {
      try_init_bmp();
    }
    if (NOT_SET(FLAG_FLASH_OK)) {
      try_init_flash();
    }
  }
}
void base_periodic_event()
{
  float t_buf = 0, p_buf = 0;
  DateTime date_buf;
  for (int i=0; (RTC_ReadDateTime(&date_buf, DEFAULT_TIMEOUT) != HAL_OK) && (i < 5); ++i) {
    debug_printf("RTC time read failed!\r\n");
    memset(&date_buf, 0, sizeof(date_buf)); // protect from garbage readings
    HAL_Delay(500);
  }
  if (IS_SET(FLAG_BMP_OK)) {
    for (int i=0; !bmp280_read_float(&bmp280, &t_buf, &p_buf, NULL) && (i < 3); ++i) {
      debug_printf("BMP280 readout failed\r\n");
      HAL_Delay(100);
    }
    bmp280_force_measurement(&bmp280);
  }

  char buf[32];
  strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &date_buf);
  debug_printf("time now: %s\r\n", buf);
  data_period_transition(saved_counts, &date_buf, t_buf, p_buf /100); // /100 for hPa
  RAISE(FLAG_DATA_SENDING);

  // blink onboard led to show that we are alive
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_RESET);
  HAL_Delay(3);
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);


}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_RTC_IRQ)
  {
    if (NOT_SET(FLAG_RTC_ALARM))
    {
      RAISE(FLAG_RTC_ALARM);
      RAISE(FLAG_EVENT_BASE);
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
