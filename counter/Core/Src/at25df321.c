#include "at25df321.h"

SPI_HandleTypeDef *at25_hspi;
GPIO_TypeDef *at25_gpio_bus;
uint16_t at25_gpio_pin;

void at25_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio_bus, uint16_t gpio_pin) {
  at25_hspi = hspi;
  at25_gpio_bus = gpio_bus;
  at25_gpio_pin = gpio_pin;
  at25_unselect();
}

void at25_select() {
  HAL_GPIO_WritePin(at25_gpio_bus, at25_gpio_pin, GPIO_PIN_RESET);
}

void at25_unselect() {
  HAL_GPIO_WritePin(at25_gpio_bus, at25_gpio_pin, GPIO_PIN_SET);
}

HAL_StatusTypeDef at25_transmit_byte(uint8_t data) {
  return HAL_SPI_Transmit(at25_hspi, &data, 1, AT25_TIMEOUT);
}

HAL_StatusTypeDef at25_receive_byte(uint8_t *data) {
  return HAL_SPI_Transmit(at25_hspi, data, 1, AT25_TIMEOUT);
}

uint8_t at25_read_status_register() {
  uint8_t buf;
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_STATUS);
  at25_receive_byte(&buf);
  at25_unselect();
  return buf;
}
