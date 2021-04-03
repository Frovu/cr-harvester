#include "at25df321.h"

void at25_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio_bus, uint16_t gpio_pin) {
  at25_hspi = hspi;
  at25_gpio_bus = gpio_bus;
  at25_gpio_pin = gpio_pin;
  at25_unselect();
}

uint8_t at25_read_status_register() {
  uint8_t buf;
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_STATUS);
  at25_receive_byte(&buf);
  at25_unselect();
  return buf;
}

uint8_t at25_is_valid() {
  uint8_t mfid = 0, devid = 0;
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_DEV_ID);
  at25_receive_byte(&mfid);
  at25_receive_byte(&devid);
  at25_unselect();
  return mfid == 0x1F && devid == 0x47;
}
