#ifndef __AT25DF321_H
#define __AT25DF321_H

#include "stm32f1xx_hal.h"

#define AT25_BIT_WRITE_OK        (1 << 5)
#define AT25_BIT_READY           (1 << 0)

#define AT25_CMD_READ_STATUS     0x05
#define AT25_CMD_READ_DEV_ID     0x9f
#define AT25_CMD_WRITE_STATUS_1  0x01
#define AT25_CMD_WRITE_ENABLE    0x06
#define AT25_CMD_WRITE_DISABLE   0x04
#define AT25_CMD_ERASE_4K        0x20
#define AT25_CMD_ERASE_32K       0x52
#define AT25_CMD_ERASE_64K       0xd8
#define AT25_CMD_ERASE_ALL       0x60
#define AT25_CMD_READ_ARRAY      0x03 // 0x0B opcode could also be used, see datasheet
#define AT25_CMD_BYTE_PROGRAM    0x02

#define AT25_TIMEOUT             250u
#define AT25_ERASE_TIMEOUT      3000u
#define AT25_PAGE_SIZE           256u
#define AT25_PAGES_COUNT        16384

// requires tickstart and timeout to be declared
#define WAIT_READY()                           \
  do {                                         \
    if (HAL_GetTick() - tickstart > timeout) { \
      return 0;                                \
    }                                          \
  } while (!at25_is_ready())

SPI_HandleTypeDef *at25_hspi;
GPIO_TypeDef *at25_gpio_bus;
uint16_t at25_gpio_pin;

static inline void at25_select() {
  HAL_GPIO_WritePin(at25_gpio_bus, at25_gpio_pin, GPIO_PIN_RESET);
}

static inline void at25_unselect() {
  HAL_GPIO_WritePin(at25_gpio_bus, at25_gpio_pin, GPIO_PIN_SET);
}

static inline HAL_StatusTypeDef at25_transmit_byte(uint8_t data) {
  return HAL_SPI_Transmit(at25_hspi, &data, 1, AT25_TIMEOUT);
}

static inline HAL_StatusTypeDef at25_receive_byte(uint8_t *data) {
  return HAL_SPI_Receive(at25_hspi, data, 1, AT25_TIMEOUT);
}

static inline void at25_write_enable() {
  at25_select();
  at25_transmit_byte(AT25_CMD_WRITE_ENABLE);
  at25_unselect();
}

static inline void at25_write_disable() {
  at25_select();
  at25_transmit_byte(AT25_CMD_WRITE_DISABLE);
  at25_unselect();
}

void at25_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio_bus, uint16_t gpio_pin);
uint8_t at25_read_status_register();
uint8_t at25_is_ready();
uint8_t at25_write_ok();
uint8_t at25_is_valid();

// block protection is not used hence not implemented here

void at25_global_unprotect();
void at25_erase(uint16_t from_page, uint16_t to_page);
uint8_t at25_erase_block(uint32_t address, uint8_t command, uint32_t timeout);
uint8_t at25_write_block(uint32_t address, uint8_t *data, uint16_t count, uint32_t timeout);
void at25_read_block(uint32_t address, uint8_t *data, uint16_t count);

#endif
