#include "at25df321.h"

SPI_HandleTypeDef *at25_hspi;
GPIO_TypeDef *at25_gpio_bus;
uint16_t at25_gpio_pin;

void init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio_bus, uint16_t gpio_pin) {
  at25_hspi = hspi;
  at25_gpio_bus = gpio_bus;
  at25_gpio_pin = gpio_pin;
  at25_unselect();
}

inline void at25_select() {
  HAL_GPIO_WritePin(at25_gpio_bus, at25_gpio_pin, GPIO_PIN_RESET);
}

inline void at25_unselect() {
  HAL_GPIO_WritePin(at25_gpio_bus, at25_gpio_pin, GPIO_PIN_SET);
}

inline HAL_StatusTypeDef at25_transmit_byte(uint8_t *char) {
  return HAL_SPI_Transmit(at25_hspi, char, 1, AT25_TIMEOUT);
}

inline HAL_SPI_Receive at25_receive_byte(uint8_t *char) {
  return HAL_SPI_Transmit(at25_hspi, char, 1, AT25_TIMEOUT);
}
