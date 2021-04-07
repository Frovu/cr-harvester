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
    LED_OFF(LED_ERROR);
    LED_ON(LED_DATA);
    // init_read_flash(); // FIXME: uncomment
    LED_OFF(LED_DATA);
    RAISE(FLAG_FLASH_OK);
    debug_printf("AT25DF321 init success\r\n");
    return 1;
  } else {
    debug_printf("AT25DF321 invalid\r\n");
    return 0;
  }
}

uint8_t try_init_rtc() {
  if (IS_SET(FLAG_RTC_OK))
    return 1;
  if (RTC_init(&hi2c2, RTC_DEFAULT_ADDR, RTC_CONFIG, DEFAULT_TIMEOUT) == HAL_OK) {
    uint8_t alarm_data[] = RTC_ALARM_CONFIG;
    if (RTC_ConfigAlarm(RTC_REG_ALARM2, alarm_data, DEFAULT_TIMEOUT) == HAL_OK) {
      if (RTC_ClearAlarm(DEFAULT_TIMEOUT) == HAL_OK) {
        RAISE(FLAG_RTC_OK);
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
  for(uint16_t i=0; i<5; ++i) {
    LED_BLINK(LED_DATA, 30);
    HAL_Delay(30);
  }
  LED_ON(LED_ERROR);
  // ******************** BMP280 ********************
  bmp280_init_default_params(&bmp280.params);
  bmp280.addr = BMP280_I2C_ADDRESS_0;
  bmp280.i2c = &hi2c2;
  for (int i=0; !try_init_bmp() && i < 3; ++i) {
    HAL_Delay(300);
    LED_BLINK_INV(LED_ERROR, 200);
  }
  // ******************** DS3231 ********************
  while (!try_init_rtc()) {
    HAL_Delay(100);
    LED_BLINK_INV(LED_ERROR, 400);
  }
  // ******************* AT25DF321 ******************
  at25_init(&hspi1, AT25_CS_GPIO_Port, AT25_CS_Pin);
  for (int i=0; !try_init_flash() && i < 10; ++i) {
    HAL_Delay(100);
    LED_BLINK_INV(LED_ERROR, 100);
  }
  LED_OFF(LED_ERROR);
}

void event_loop() {
  if (IS_SET(FLAG_RTC_OK))
  {
    if (IS_SET(FLAG_EVENT_BASE))
    {
      last_period_tick = HAL_GetTick();
      base_periodic_event();
      TOGGLE(FLAG_EVENT_BASE);
    }
    if (IS_SET(FLAG_RTC_ALARM))
    {
      uint16_t alarm_reset = 0;
      for(uint16_t i=0; i<3; ++i) {
        // try reset alarm 3 times, if failed raise RTC reinitializaiton flag
        if (RTC_ClearAlarm(50) == HAL_OK) {
          alarm_reset = 1;
          break;
        } else {
          LED_BLINK(LED_ERROR, 50);
          debug_printf("can't reset RTC alarm\r\n");
        }
      }
      if (alarm_reset) {
        TOGGLE(FLAG_RTC_ALARM);
      } else {
        TOGGLE(FLAG_RTC_OK); // reset RTC OK flag
      }
    }
    else
    { /* If RTC does not produce alarm IRQ for more than a minute,
      *  it means something is wrong with RTC and the only thing we
      *  can do here is just to try to re-initialize RTC's alarm registers
      *  with some period and hope it will be restored in its place
      */
      if (HAL_GetTick() - last_period_tick > BASE_EVENT_WATCHDOG_MS) {
        TOGGLE(FLAG_RTC_OK);
        debug_printf("period watchdog triggered\r\n");
      }
    }
  }
  else // try to fix rtc somehow
  {
    LED_OFF(LED_ERROR);
    if (!try_init_rtc()) {
      HAL_I2C_Init(&hi2c2);
      HAL_Delay(300);
      LED_ON(LED_ERROR);
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
        LED_OFF(LED_DATA);
      } else if (storage_stat < 0) {
        // if failed to send line wait until something probably fixes idk
        LED_BLINK_INV(LED_DATA, 30);
        LED_BLINK(LED_ERROR, 30);
        HAL_Delay(1000); // FIXME: probably too much
      }
    }
    if (NOT_SET(FLAG_BMP_OK)) {
      if (!try_init_bmp()) {
        HAL_I2C_Init(&hi2c2);
        HAL_Delay(500);
        LED_BLINK(LED_ERROR, 30);
      }
    }
    if (NOT_SET(FLAG_FLASH_OK)) {
      if (!try_init_flash()) {
        HAL_Delay(500);
        LED_BLINK(LED_ERROR, 30);
      }
    }
  }
}
void base_periodic_event()
{
  uint16_t flag_ok = 0;
  float t_buf = 0, p_buf = 0;
  DateTime date_buf;
  if (IS_SET(FLAG_RTC_OK)) {
    for (int i=0; i < 3; ++i) {
      if (RTC_ReadDateTime(&date_buf, DEFAULT_TIMEOUT) == HAL_OK) {
        flag_ok = 1;
        break;
      }
      debug_printf("RTC time read failed!\r\n");
      memset(&date_buf, 0, sizeof(date_buf)); // protect from garbage readings
      HAL_Delay(300);
    }
    if (!flag_ok) { // RTC is lost
      TOGGLE(FLAG_RTC_OK);
    }
  }

  if (IS_SET(FLAG_BMP_OK)) {
    flag_ok = 0;
    for (int i=0; i < 3; ++i) {
      if (bmp280_read_float(&bmp280, &t_buf, &p_buf, NULL)) {
        flag_ok = 1;
        break;
      }
      debug_printf("BMP280 readout failed\r\n");
      HAL_Delay(200);
    }
    if (flag_ok) {
      bmp280_force_measurement(&bmp280);
    } else { // BMP is lost
      TOGGLE(FLAG_BMP_OK);
    }
  }

  char buf[32];
  strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &date_buf);
  debug_printf("time now: %s\r\n", buf);
  data_period_transition(saved_counts, &date_buf, t_buf, p_buf /100); // /100 for hPa

  RAISE(FLAG_DATA_SENDING);
  LED_ON(LED_DATA);

  // blink onboard led to show that we are alive
  LED_BLINK_INV(BOARD_LED, 10);

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
