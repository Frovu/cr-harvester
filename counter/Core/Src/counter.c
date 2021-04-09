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
uint32_t last_fix_attempt = 0;
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

/* Core device algorithm is described in this function
*  it repeatedly reads flags variable and takes appropriate actions
*/
void event_loop() {
  if (IS_SET(FLAG_RTC_OK))
  { /* Counter registration logic may work if and only if RTC is connected, alive
    *  and produces interrupt signals on every period transition
    */
    if (IS_SET(FLAG_EVENT_BASE))
    {
      base_periodic_event();
      TOGGLE(FLAG_EVENT_BASE);
      RAISE(FLAG_DATA_SENDING); // there is new data to be sent
    }
    if (IS_SET(FLAG_RTC_ALARM))
    { /* RTC alarm flag essentially repeats BASE EVENT flag, but its made to protect
      *  from accidential noise interrupts which may occur in case of electrical
      *  connection problems. Alarm flag forbids BASE flag to be set again
      *  until alarm bit in RTC register is successfully reset
      */
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
      *  it means that either RTC is mis-configured or I2C bus
      *  connection is lost or corrupt
      */
      if (HAL_GetTick() - last_period_tick > BASE_EVENT_WATCHDOG_MS) {
        TOGGLE(FLAG_RTC_OK);
        debug_printf("period watchdog triggered\r\n");
      }
    }
  }
  /* *********************** DATA-SENDING SECTION *********************** */
  if (!W5500_Connected())
  { /* Reassure that w5500 is connected to avoid freezes */
    if (IS_SET(FLAG_W5500_OK)) {
      TOGGLE(FLAG_W5500_OK);
    }
  }
  int32_t time_left = BASE_PERIOD_LEN_MS - HAL_GetTick() + last_period_tick;
  if (time_left > SENDING_TIMEOUT * 2)
  { /* Reassure that we have enough time before next period, since failed sending try
    *  can possibly take fair amount of time due to big timeouts
    */
    if (IS_SET(FLAG_DATA_SENDING) && IS_SET(FLAG_W5500_OK) && NOT_SET(FLAG_DHCP_RUN))
    {
      // TODO: complex data-sending error handling
      int32_t storage_stat = data_send_one(SENDING_TIMEOUT);
      if (storage_stat == 0) {
        // everything sent
        TOGGLE(FLAG_DATA_SENDING);
        LED_OFF(LED_DATA);
      } else if (storage_stat < 0) {
        // if failed to send line wait until something probably fixes idk
        RAISE(FLAG_DHCP_RUN);
        LED_BLINK_INV(LED_DATA, 30);
        LED_BLINK(LED_ERROR, 100);
      }
    }
  }
  /* ********************* SOMETHING-NOT-OK SECTION ********************* */
  if (IS_SET(FLAG_W5500_OK) && IS_SET(FLAG_DHCP_RUN))
  { /* The DHCP client is ran only when corresponding flag is set
    */
    if (W5500_RunDHCP()) {
      TOGGLE(FLAG_DHCP_RUN);
    }
  }
  uint32_t since_last_fix_attempt = HAL_GetTick() - last_fix_attempt;
  // incorporate non-blocking delay for PROBLEM_FIXING_PERIOD ms
  if (since_last_fix_attempt > PROBLEM_FIXING_PERIOD && time_left > (PROBLEM_FIXING_PERIOD * 2))
  {
    if (NOT_SET(FLAG_RTC_OK))
    { /* To restore RTC functionality the only thing we can do is repeatedly try
      *  to re-initialize I2C peripheral and RTC device itself
      */
      if (!try_init_dev(DEV_RTC)) {
        HAL_I2C_Init(&hi2c2);
      }
    }
    if (NOT_SET(FLAG_BMP_OK)) {
      if (!try_init_dev(DEV_BMP)) {
        HAL_I2C_Init(&hi2c2);
      }
    }
    if (NOT_SET(FLAG_FLASH_OK)) {
      try_init_dev(DEV_FLASH);
    }
    if (NOT_SET(FLAG_W5500_OK)) {
      // TODO: hardware reset
      try_init_dev(DEV_W5500);
    }
    /* Blink ERROR led for n times depending of dev_ok flag bits
    * so 0 means everything ok and 15 means everything is bad
    */
    uint16_t blinks = ((flags & FLAG_OK_MASK) ^ FLAG_OK_MASK) >> FLAG_OK_SHIFT;
    if (blinks > 0) {
      last_fix_attempt = HAL_GetTick(); // save tick for non-blocking delay
    }
    for (uint16_t i = 0; i < blinks; ++i) {
      LED_BLINK(LED_ERROR, 10);
      HAL_Delay(150);
    }
  }
  /* *********************** ******************** *********************** */
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

  data_period_transition(saved_counts, &last_period_tm, t_buf, p_buf /100); // /100 for hPa

  LED_ON(LED_DATA);

  // blink onboard led to show that we are alive
  LED_BLINK_INV(BOARD_LED, 10);

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_RTC_IRQ)
  {
    if (NOT_SET(FLAG_RTC_ALARM))
    { // save RTC minute start tick to extend local time precision up to ms
      last_period_tick = HAL_GetTick();
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
