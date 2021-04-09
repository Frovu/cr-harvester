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

volatile uint16_t flags = FLAGS_INITIAL;

volatile uint16_t saved_counts[CHANNELS_COUNT];
volatile uint16_t counters[CHANNELS_COUNT];

uint32_t cycle_counter = 0;
uint32_t last_period_tick = 0;
DateTime last_period_tm;

uint8_t try_init_dev(device_t dev)
{
  uint16_t flagVal = 0;
  uint8_t status = 0;
  switch (dev) {
  case DEV_RTC:
    flagVal = FLAG_RTC_OK;
    uint8_t alarm_data[] = RTC_ALARM_CONFIG;
    status = RTC_init(&hi2c2, RTC_DEFAULT_ADDR, RTC_CONFIG, DEFAULT_TIMEOUT) == HAL_OK;
    status = status && (RTC_ConfigAlarm(RTC_ALARM_REG, alarm_data, DEFAULT_TIMEOUT) == HAL_OK);
    status = status && (RTC_ClearAlarm(DEFAULT_TIMEOUT) == HAL_OK);
    break;
  case DEV_BMP:
    flagVal = FLAG_BMP_OK;
    status = bmp280_init(&bmp280, &bmp280.params);
    break;
  case DEV_FLASH:
    flagVal = FLAG_FLASH_OK;
    status = at25_is_valid() && at25_is_ready();
    if (status) {
      at25_global_unprotect();
      LED_OFF(LED_ERROR);
      LED_ON(LED_DATA);
      // init_read_flash(); // FIXME: uncomment
      LED_OFF(LED_DATA);
    }
    break;
  case DEV_W5500:
    flagVal = FLAG_W5500_OK;
    status = W5500_Init();
    break;
  }
  if (status) // initialization successful
  {
    RAISE(flagVal);
  }
  debug_printf("init: %s\t%s\r\n", dev == DEV_RTC ? "RTC" : dev == DEV_BMP ? "BMP280"
    : dev == DEV_FLASH ? "FLASH" : dev == DEV_W5500 ? "W5500" : "x", status ? "OK" : "FAIL");
  return status;
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
  for (int i=0; !try_init_dev(DEV_BMP) && i < 3; ++i) {
    HAL_Delay(300);
    LED_BLINK_INV(LED_ERROR, 200);
  }
  // ******************** DS3231 ********************
  while (!try_init_dev(DEV_RTC)) {
    HAL_Delay(100);
    LED_BLINK_INV(LED_ERROR, 400);
  }
  // ******************* AT25DF321 ******************
  at25_init(&hspi1, AT25_CS_GPIO_Port, AT25_CS_Pin);
  for (int i=0; !try_init_dev(DEV_FLASH) && i < 5; ++i) {
    HAL_Delay(100);
    LED_BLINK_INV(LED_ERROR, 200);
  }
  // ******************* W5500 **********************
  for (int i=0; !try_init_dev(DEV_W5500) && i < 3; ++i) {
    HAL_Delay(300);
    LED_BLINK_INV(LED_ERROR, 600);
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
      try_sync_ntp(3000);
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
    if (!try_init_dev(DEV_RTC)) {
      HAL_I2C_Init(&hi2c2);
      HAL_Delay(300);
      LED_ON(LED_ERROR);
    }
  }
  if (IS_SET(FLAG_DHCP_RUN))
  {
    if (!W5500_RunDHCP()) {
      LED_BLINK(LED_ERROR, 50);
      HAL_Delay(450);
    } else {
      TOGGLE(FLAG_DHCP_RUN);
    }
  }
  int32_t time_left = BASE_PERIOD_LEN_MS - HAL_GetTick() + last_period_tick;
  if (time_left > SENDING_TIMEOUT * 2)
  {
    if (IS_SET(FLAG_DATA_SENDING) && NOT_SET(FLAG_DHCP_RUN)) {
    	int32_t storage_stat = data_send_one(SENDING_TIMEOUT);
      if (storage_stat == 0) {
        // everything sent
        TOGGLE(FLAG_DATA_SENDING);
        LED_OFF(LED_DATA);
      } else if (storage_stat < 0) {
        // if failed to send line wait until something probably fixes idk
        RAISE(FLAG_DHCP_RUN);
        LED_BLINK_INV(LED_DATA, 30);
        LED_BLINK(LED_ERROR, 500);
      }
    }
    if (NOT_SET(FLAG_BMP_OK)) {
      if (!try_init_dev(DEV_BMP)) {
        HAL_I2C_Init(&hi2c2);
        HAL_Delay(500);
        LED_BLINK(LED_ERROR, 30);
      }
    }
    if (NOT_SET(FLAG_FLASH_OK)) {
      if (!try_init_dev(DEV_FLASH)) {
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
  if (IS_SET(FLAG_RTC_OK)) {
    for (int i=0; i < 3; ++i) {
      if (RTC_ReadDateTime(&last_period_tm, DEFAULT_TIMEOUT) == HAL_OK) {
        flag_ok = 1;
        break;
      }
      debug_printf("RTC time read failed!\r\n");
      memset(&last_period_tm, 0, sizeof(last_period_tm)); // protect from garbage readings
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
  strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &last_period_tm);
  debug_printf("time now: %s\r\n", buf);
  data_period_transition(saved_counts, &last_period_tm, t_buf, p_buf /100); // /100 for hPa

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
