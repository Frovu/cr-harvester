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

volatile uint16_t flags = 0;
uint32_t cycle_counter = 0;

uint8_t try_init_bmp() {
  if (flags & FLAG_BMP_OK)
    return 1;
  if (bmp280_init(&bmp280, &bmp280.params)) {
    flags |= FLAG_BMP_OK;
    debug_printf("BMP280 init success, id = 0x%x\r\n", bmp280.id);
    bmp280_force_measurement(&bmp280);
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

void counter_init() {
  debug_printf("INIT\r\n");
  // ******************** BMP280 ********************
  bmp280_init_default_params(&bmp280.params);
  bmp280.addr = BMP280_I2C_ADDRESS_0;
  bmp280.i2c = &hi2c2;
  // try to init bmp280 3 times with 1 second delay
  for (int i=0; !try_init_bmp() && i < 3; ++i)
    HAL_Delay(500);
  // ******************* AT25DF321 ******************
  at25_init(&hspi1, AT25_CS_GPIO_Port, AT25_CS_Pin);
  for (int i=0; !try_init_flash() && i < 3; ++i) {
    HAL_Delay(300);
  }
  // ******************** DS3231 ********************
  int8_t s = RTC_init(&hi2c2, RTC_DEFAULT_ADDR, RTC_CONTROL_A2IE|RTC_CONTROL_INTCN, DEFAULT_TIMEOUT) == HAL_OK;
  debug_printf("RTC init: %s\r\n", s ? "OK" : "FAIL");
  // TODO: retry

  base_clock_event();
}

void event_loop() {
  if (flags & FLAG_EVENT_BASE) {
    base_clock_event();
    flags ^= FLAG_EVENT_BASE;
  }
  if (flags & FLAG_RTC_ALARM) {
      uint8_t abuf[4];
//      uint8_t alarm_mask[4] = { 0x80, 0x80, 0x80, 0x80 };
//      HAL_I2C_Mem_Write(&hi2c2, RTC_DEFAULT_ADDR, RTC_REG_ALARM2, 1, alarm_mask, 3, DEFAULT_TIMEOUT);
//      HAL_I2C_Mem_Read(&hi2c2, RTC_DEFAULT_ADDR, RTC_REG_ALARM2, 1, abuf, 4, DEFAULT_TIMEOUT);
//      debug_printf("malarm 2: 0x%x 0x%x 0x%x\r\n", abuf[0], abuf[1], abuf[2]);
//      HAL_I2C_Mem_Read(&hi2c2, RTC_DEFAULT_ADDR, RTC_REG_CONTROL, 1, abuf, 2, DEFAULT_TIMEOUT);
//      debug_printf("prec alarm: 0x%x status: 0x%x\r\n", abuf[0], abuf[1]);
    if (RTC_ClearAlarm(50) == HAL_OK) {
      flags ^= FLAG_RTC_ALARM;
//      HAL_I2C_Mem_Read(&hi2c2, RTC_DEFAULT_ADDR, RTC_REG_CONTROL, 1, abuf, 2, DEFAULT_TIMEOUT);
//      debug_printf("c alarm control: 0x%x status: 0x%x\r\n", abuf[0], abuf[1]);
    }
  }

}
void base_clock_event() {
  float t, p;
  if (flags & FLAG_BMP_OK) {
	for (int i=0; i<3; ++i) {
	  if (bmp280_read_float(&bmp280, &t, &p, NULL))
		break;
	  debug_printf("BMP280 read failed\r\n");
	  HAL_Delay(500);
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
