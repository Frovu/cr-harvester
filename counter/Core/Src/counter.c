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

uint16_t flags = 0;
uint16_t seconds_counter = COUNTER_DATA_RATE - 4;
uint32_t cycle_counter = 0;

void counter_init() {
  debug_printf("INIT\r\n");
  // ******************* AT25DF321 ******************
  at25_init(&hspi1, AT25_CS_GPIO_Port, AT25_CS_Pin);
  // TODO: check status

  // ******************** BMP280 ********************
  bmp280_init_default_params(&bmp280.params);
  bmp280.addr = BMP280_I2C_ADDRESS_0;
  bmp280.i2c = &hi2c2;
  // try to init bmp280 3 times with 1 second delay
  for (int i=0; i < 3; ++i) {
    if (bmp280_init(&bmp280, &bmp280.params)) {
      flags |= FLAG_BMP_OK;
      debug_printf("BMP280 init success, id = 0x%x\r\n", bmp280.id);
      bmp280_force_measurement(&bmp280);
      break;
    } else {
      debug_printf("BMP280 init failed\r\n");
      HAL_Delay(1000);
    }
  }
}

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
}

void base_clock_event() {
  // blink onboard led to show that we are alive
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_RESET);
  HAL_Delay(3);
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);

  debug_printf("at25df321 validity = %x\r\n", at25_is_valid());
  debug_printf("at25df321 status = 0x%x\r\n", at25_read_status_register());
  at25_write_enable();
  debug_printf("write enable\r\n");
  debug_printf("at25df321 status = 0x%x\r\n", at25_read_status_register());
  at25_write_disable();
  debug_printf("write disable\r\n");

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
