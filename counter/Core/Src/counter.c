/**
  ******************************************************************************
  * @file           : counter.c
  * @brief          : Core counter logic
  ******************************************************************************
*/
#include "counter.h"

extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern ADC_HandleTypeDef hadc1;

BMP280_HandleTypedef bmp280;

Configuration * cfg;
uint16_t flags = FLAGS_INITIAL;

volatile uint16_t saved_counts[CHANNELS_COUNT];
volatile uint16_t counters[CHANNELS_COUNT];

int8_t lookup_channel_idx[16] = { [0 ... 15] = -1 }; // 16 GPIO channels

uint32_t second_counter = 0;
uint32_t cycle_counter = 0;
uint32_t last_ntp_sync = 0;

int64_t last_period_tick = 0;
uint32_t last_fix_attempt = -5000;
uint32_t last_net_attempt = -5000;
uint32_t last_dns_attempt = -5000;
uint32_t last_srv_attempt = -5000;
DateTime last_period_tm;

float read_adc(void) {
  uint32_t raw, sum = 0;
  for (uint32_t i=0; i < ADC_OVERSAMPLING; ++i) {
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 500) == HAL_OK) {
      raw = HAL_ADC_GetValue(&hadc1);
      sum += raw;
    } else {
      debug_printf("ADC readout failed\r\n");
      return -1;
    }
    HAL_Delay(ADC_SAMPLING_DELAY);
  }
  HAL_ADC_Stop(&hadc1);
  return ((float) sum  * ADC_PRESCALER / ADC_OVERSAMPLING) * 3.3 / 4096;
}

uint8_t try_init_dev(device_t dev)
{
  uint16_t flagVal = 0;
  uint8_t status = 0;
  switch (dev) {
  case DEV_DS18B20:
    flagVal = FLAG_DS18B20_OK;
    status = ds18b20_init() == HAL_OK;
    if (status) {
      ds18b20_start_conversion();
    }
    break;
  case DEV_BMP:
    flagVal = FLAG_BMP_OK;
    status = bmp280_init(&bmp280, &bmp280.params);
    if (status) {
      bmp280_force_measurement(&bmp280);
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
  debug_printf("init: %s\t%s\r\n", dev == DEV_DS18B20 ? "DS18B20" : dev == DEV_BMP ? "BMP280"
    : dev == DEV_W5500 ? "W5500" : "x", status ? "OK" : "FAIL");
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
  LED_ON(BOARD_LED);
  LED_ON(LED_ERROR);
  cfg = malloc(sizeof(Configuration));
  memcpy(cfg, &default_cfg, sizeof(Configuration));
  debug_printf("sizeof(DataLine) = %d\r\n", sizeof(DataLine));
  debug_printf("DATA_BUFFER_LEN = %d\r\n", DATA_BUFFER_LEN);
  bool adc_ok = HAL_ADCEx_Calibration_Start(&hadc1) == HAL_OK;
  debug_printf("init: ADC\t%s\r\n", adc_ok ? "OK" : "FAIL");
  // ******************** BMP280 ********************
  bmp280_init_default_params(&bmp280.params);
  bmp280.addr = BMP280_I2C_ADDRESS_0;
  bmp280.i2c = &hi2c1;
  for (int i=0; !try_init_dev(DEV_BMP) && i < 3; ++i) {
    HAL_Delay(100);
    LED_BLINK_INV(LED_ERROR, 200);
  }
  // ******************** DS18B20 ********************
  OW_INIT();
  HAL_Delay(100);
  for (int i=0; !try_init_dev(DEV_DS18B20) && i < 3; ++i) {
    HAL_Delay(100);
    LED_BLINK_INV(LED_ERROR, 200);
  }
  // ******************* W5500 **********************
  for (int i=0; !try_init_dev(DEV_W5500) && i < 5; ++i) {
    HAL_Delay(300);
    LED_BLINK_INV(LED_ERROR, 600);
  }
  LED_OFF(LED_ERROR);
  // Fill the Channels EXTI GPIO reverse lookup table
  for(uint16_t i=0; i<CHANNELS_COUNT; ++i) {
    lookup_channel_idx[GPIO_TO_N(CHANNELS_GPIO_LIST[i])] = i;
  }
}

/* Core device algorithm is described in this function
*  it repeatedly reads flags variable and takes appropriate actions
*/
void event_loop() {
  if (IS_SET(FLAG_EVENT_BASE))
  {
    base_periodic_event();
    if (IS_SET(FLAG_PRE_PERIOD))
      TOGGLE(FLAG_PRE_PERIOD);
    TOGGLE(FLAG_EVENT_BASE);
    RAISE(FLAG_DATA_SENDING); // there is new data to be sent
    LED_ON(LED_DATA);
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
  if (time_left > (SENDING_TIMEOUT + DEFAULT_TIMEOUT))
  { /* Reassure that we have enough time before next period, since failed sending try
    *  can possibly take fair amount of time due to big timeouts
    */
    if (IS_SET(FLAG_DATA_SENDING) && IS_SET(FLAG_W5500_OK) && NOT_SET(FLAG_DHCP_RUN) && since_last_attempt > NET_FIXING_PERIOD)
    {
      switch (data_send_one(SENDING_TIMEOUT)) {
        case DATA_CLEAR:
          TOGGLE(FLAG_DATA_SENDING);
          LED_BLINK(LED_DATA, 200);
          break;
        case DATA_OK:
          LED_BLINK_INV(LED_DATA, 30);
          break;
        case DATA_NET_ERROR:
          if (IS_SET(FLAG_W5500_OK))
            TOGGLE(FLAG_W5500_OK);
          // no break here
        case DATA_NET_TIMEOUT:
          if (cfg->dhcp_mode == NETINFO_DHCP) {
            RAISE(FLAG_DHCP_RUN);
          }
          RAISE(FLAG_DNS_RUN);
          // no break here
        case DATA_NET_NOT_OK:
          last_net_attempt = HAL_GetTick();
          LED_BLINK_INV(LED_DATA, 100);
          LED_BLINK(LED_ERROR, 150);
      }
    }
    /* ********************* DHCP / DNS RUN SECTION *********************** */
    if (IS_SET(FLAG_W5500_OK)) {
      if (IS_SET(FLAG_DHCP_RUN))
      { /* The DHCP client is ran repeatedly when corresponding flag is set */
        int8_t dhcr = W5500_RunDHCP();
        if (dhcr > 0) {
          TOGGLE(FLAG_DHCP_RUN);
        } else if (dhcr < 0) {
          TOGGLE(FLAG_W5500_OK);
        }
      }
      if (IS_SET(FLAG_DNS_RUN) && NOT_SET(FLAG_DHCP_RUN) &&  (HAL_GetTick()-last_dns_attempt > 500))
      { /* The DNS client is ran repeatedly when corresponding flag is set */
        if (run_dns_queries()) {
          TOGGLE(FLAG_DNS_RUN);
        } else {
          last_dns_attempt = HAL_GetTick();
        }
      }
    }
  }
  /* ****************** SOMETHING-NOT-OK / NTP SECTION ****************** */
  uint32_t since_last_fix_attempt = HAL_GetTick() - last_fix_attempt;
  // incorporate non-blocking delay for PROBLEM_FIXING_PERIOD ms
  if (since_last_fix_attempt > PROBLEM_FIXING_PERIOD && time_left > (PROBLEM_FIXING_PERIOD * 2))
  {
    if (IS_SET(FLAG_NTP_SYNC) && IS_SET(FLAG_W5500_OK)
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
    if (NOT_SET(FLAG_BMP_OK)) {
      if (!try_init_dev(DEV_BMP)) {
        HAL_I2C_Init(&hi2c1);
      }
    }
    if (NOT_SET(FLAG_DS18B20_OK)) {
      try_init_dev(DEV_DS18B20);
    }
    if (NOT_SET(FLAG_W5500_OK)) {
      HAL_GPIO_WritePin(W5500_RESET_GPIO_Port, W5500_RESET_Pin, GPIO_PIN_RESET);
      HAL_Delay(100);
      HAL_GPIO_WritePin(W5500_RESET_GPIO_Port, W5500_RESET_Pin, GPIO_PIN_SET);
      HAL_Delay(100);
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
  float t_buf = 0, p_buf = 0, te_buf = 0, v_buf = 0;
  for (int i=0; i < 3; ++i) {
    if (RTC_ReadDateTime(&last_period_tm, DEFAULT_TIMEOUT) == HAL_OK) {
      flag_ok = 1;
      break;
    }
    memset(&last_period_tm, 0, sizeof(last_period_tm)); // protect from garbage readings
  }
  if (!flag_ok) { // RTC is lost
    debug_printf("RTC readout failed\r\n");
  }
  /* Since rtc every minute alarm is used, the seconds counter on the period begiining
   * should always be equal to zero, so we can just zero it out in data struct in case
   * it was read after some time due to data sending and other possible delays */
  last_period_tm.tm_sec = 0; // FIXME: if base period not equal to minute!

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
  if (IS_SET(FLAG_DS18B20_OK)) {
    flag_ok = 0;
    for (int i=0; i < 3; ++i) {
      if (ds18b20_read_temperature(&te_buf) == HAL_OK) {
        flag_ok = 1;
        break;
      }
      debug_printf("DS18B20 readout failed\r\n");
      HAL_Delay(200);
    }
    if (flag_ok) {
      ds18b20_start_conversion();
    } else { // lost sensot
      TOGGLE(FLAG_DS18B20_OK);
    }
  }

  v_buf = read_adc();

  data_period_transition(saved_counts, &last_period_tm, t_buf, p_buf/100, te_buf, v_buf); // /100 for hPa

  // blink onboard led to show that we are alive
  LED_BLINK_INV(BOARD_LED, 10);

}

void RTC_IRQ_Callback() {
  if (++second_counter > 59) {
    // save RTC "minute" start tick to extend local time precision up to ms
    last_period_tick = HAL_GetTick();
    RAISE(FLAG_EVENT_BASE);
    for (uint8_t i=0; i<CHANNELS_COUNT; ++i) {
      saved_counts[i] = counters[i];
    }
    ++cycle_counter;
    second_counter = 0;
  }
  debug_printf("[ %02d:%02d:%02d .%03d ]\r\n", last_period_tm.tm_hour, last_period_tm.tm_min, second_counter, HAL_GetTick() % 1000);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  int8_t idx = lookup_channel_idx[GPIO_TO_N(GPIO_Pin)];
  if (idx >= 0)
  {
    ++counters[idx];
  }
}
