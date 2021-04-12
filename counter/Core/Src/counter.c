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

uint16_t flags = FLAGS_INITIAL;

volatile uint16_t saved_counts[CHANNELS_COUNT];
volatile uint16_t counters[CHANNELS_COUNT];

uint32_t cycle_counter = 0;
uint32_t last_ntp_sync = 0;

int64_t last_period_tick = 0;
uint32_t last_fix_attempt = -5000;
uint32_t last_net_attempt = -5000;
uint32_t last_srv_attempt = -5000;
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
      RAISE(FLAG_FLASH_INIT);
    }
    break;
  case DEV_W5500:
    flagVal = FLAG_W5500_OK;
    status = W5500_Init();
    if (status) {
      if (cfg->dhcp_mode == NETINFO_DHCP) {
        RAISE(FLAG_DHCP_RUN);
      }
      RAISE(FLAG_DNS_RUN);
    }
    break;
  }
  if (status) // initialization successful
  {
    RAISE(flagVal);
  }
  #ifdef DEBUG_UART
  debug_printf("init: %s\t%s\r\n", dev == DEV_RTC ? "RTC" : dev == DEV_BMP ? "BMP280"
    : dev == DEV_FLASH ? "FLASH" : dev == DEV_W5500 ? "W5500" : "x", status ? "OK" : "FAIL");
  #endif
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
  // ******************* AT25DF321 ******************
  at25_init(&hspi1, AT25_CS_GPIO_Port, AT25_CS_Pin);
  for (int i=0; !try_init_dev(DEV_FLASH) && i < 5; ++i) {
    HAL_Delay(100);
    LED_BLINK_INV(LED_ERROR, 200);
  }
  // ******************* CONFIG *********************
  if (IS_SET(FLAG_FLASH_OK)) {
    /* Set and save default settings if RESET button is pressed */
    if (HAL_GPIO_ReadPin(BUTTON_RESET_GPIO_Port, BUTTON_RESET_Pin) == GPIO_PIN_RESET) {
      debug_printf("INIT DEFAULT CONFIG\r\n");
      config_set_default();
      config_save();
      for(uint16_t i=0; i<16; ++i) {
        LED_BLINK(LED_DATA,  50);
        LED_BLINK(LED_ERROR, 50);
      }
    } else {
      config_initialize();
    }
  } else {
    config_set_default();
  }
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
  // ******************* W5500 **********************
  for (int i=0; !try_init_dev(DEV_W5500) && i < 3; ++i) {
    HAL_Delay(300);
    LED_BLINK_INV(LED_ERROR, 600);
  }
  /* Initial NTP sync happens before first cycle, hence requires
  *  setting last_period_tick/tm explicitly
  */
  if (RTC_ReadDateTime(&last_period_tm, DEFAULT_TIMEOUT) != HAL_OK) {
    if (IS_SET(FLAG_RTC_OK)) {
      TOGGLE(FLAG_RTC_OK);
    }
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
      if (IS_SET(FLAG_PRE_PERIOD))
        TOGGLE(FLAG_PRE_PERIOD);
      TOGGLE(FLAG_EVENT_BASE);
      RAISE(FLAG_DATA_SENDING); // there is new data to be sent
      LED_ON(LED_DATA);
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
  /* ************************ TIMEKEEPIG SECTION ************************ */
  if (cycle_counter - last_ntp_sync > NTP_SYNC_PERIOD)
  { /* Request ntp sync if more than NTP_SYNC_PERIOD passed since last sync */
    RAISE(FLAG_NTP_SYNC);
  }
  if (IS_SET(FLAG_TIME_TRUSTED))
  { /* Mark local time untrusted if more than TIME_TRUST_PERIOD passed since last ntp sync */
    if (cycle_counter - last_ntp_sync > TIME_TRUST_PERIOD) {
      TOGGLE(FLAG_TIME_TRUSTED);
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
  uint32_t since_last_attempt = HAL_GetTick() - last_net_attempt; // non-blocking delay for net failures
  /* Perform initial read of external flash. This operation takes alot of time so
   * its better to do it after NTP sync to not loose one data period */
  if (IS_SET(FLAG_FLASH_INIT) && (NOT_SET(FLAG_NTP_SYNC) || NOT_SET(FLAG_PRE_PERIOD))) {
    if (time_left > FLASH_INIT_TIME && NOT_SET(FLAG_EVENT_BASE)) {
      init_read_flash();
      TOGGLE(FLAG_FLASH_INIT);
    }
  }
  if (time_left > SENDING_TIMEOUT * 2)
  { /* Reassure that we have enough time before next period, since failed sending try
    *  can possibly take fair amount of time due to big timeouts
    */
    if (IS_SET(FLAG_DATA_SENDING) && IS_SET(FLAG_W5500_OK) && NOT_SET(FLAG_DHCP_RUN) && since_last_attempt > 1000)
    {
      switch (data_send_one(SENDING_TIMEOUT)) {
        case DATA_CLEAR:
          TOGGLE(FLAG_DATA_SENDING);
          LED_BLINK(LED_DATA, 200);
          break;
        case DATA_OK:
          LED_BLINK_INV(LED_DATA, 30);
          break;
        case DATA_FLASH_ERROR:
          if (IS_SET(FLAG_FLASH_OK))
            TOGGLE(FLAG_FLASH_OK);
          break;
        case DATA_NET_ERROR:
          if (IS_SET(FLAG_W5500_OK))
            TOGGLE(FLAG_W5500_OK);
          // no break here
        case DATA_NET_TIMEOUT:
          if (cfg->dhcp_mode == NETINFO_DHCP) {
            RAISE(FLAG_DHCP_RUN);
          }
          // no break here
        case DATA_NET_NOT_OK:
          last_net_attempt = HAL_GetTick();
          LED_BLINK_INV(LED_DATA, 30);
          LED_BLINK(LED_ERROR, 100);
      }
    }
    /* ********************* DHCP / DNS RUN SECTION *********************** */
    if (IS_SET(FLAG_W5500_OK)) {
      if (IS_SET(FLAG_DHCP_RUN))
      { /* The DHCP client is ran repeatedly when corresponding flag is set */
        if (W5500_RunDHCP()) {
          TOGGLE(FLAG_DHCP_RUN);
        }
      }
      if (IS_SET(FLAG_DNS_RUN) && NOT_SET(FLAG_DHCP_RUN) && since_last_attempt > 5000)
      { /* The DNS client is ran repeatedly when corresponding flag is set */
        if (run_dns_queries()) {
          TOGGLE(FLAG_DNS_RUN);
        } else {
          last_net_attempt = HAL_GetTick();
        }
      }
    }
  }
  /* *********************** CONFIG HTTP SERVER ************************* */
  if (IS_SET(FLAG_W5500_OK) && (HAL_GetTick() - last_srv_attempt) > 1000) {
    switch (config_server_run()) {
      case DATA_OK:
        break;
      case DATA_CLEAR:
       /*  User changed device configuration, we should re-initialize W5500 in case
        *  of DHCP mode change, re-run DHCP, DNS, NTP queries if servers changed
        */
        TOGGLE(FLAG_W5500_OK); // reinit W5500
        RAISE(FLAG_DHCP_RUN);
        RAISE(FLAG_DNS_RUN);
        RAISE(FLAG_NTP_SYNC);
        break;
      default:
        last_srv_attempt = HAL_GetTick();
        break;
    }
  }
  /* ****************** SOMETHING-NOT-OK / NTP SECTION ****************** */
  uint32_t since_last_fix_attempt = HAL_GetTick() - last_fix_attempt;
  // incorporate non-blocking delay for PROBLEM_FIXING_PERIOD ms
  if (since_last_fix_attempt > PROBLEM_FIXING_PERIOD && time_left > (PROBLEM_FIXING_PERIOD * 2))
  {
    if (IS_SET(FLAG_NTP_SYNC) && IS_SET(FLAG_RTC_OK) && IS_SET(FLAG_W5500_OK)
        && NOT_SET(FLAG_DNS_RUN) && NOT_SET(FLAG_DHCP_RUN))
    { /* Syncronize local RTC to NTP time */
      if (try_sync_ntp(SENDING_TIMEOUT)) {
        TOGGLE(FLAG_NTP_SYNC);
        RAISE(FLAG_TIME_TRUSTED);
        last_ntp_sync = cycle_counter;
      } else {
        last_fix_attempt = HAL_GetTick(); // non-blocking delay of PROBLEM_FIXING_PERIOD
      }
    }
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
