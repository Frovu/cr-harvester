#include "at25df321.h"

void at25_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *gpio_bus, uint16_t gpio_pin)
{
  at25_hspi = hspi;
  at25_gpio_bus = gpio_bus;
  at25_gpio_pin = gpio_pin;
  at25_unselect();
}

uint8_t at25_read_status_register()
{
  uint8_t buf;
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_STATUS);
  at25_receive_byte(&buf);
  at25_unselect();
  return buf;
}

uint8_t at25_is_ready()
{
  return (at25_read_status_register() & AT25_BIT_READY) == 0;
}

uint8_t at25_write_ok() {
  return (at25_read_status_register() & AT25_BIT_WRITE_OK) == 0;
}

uint8_t at25_is_valid()
{
  uint8_t mfid = 0, devid = 0;
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_DEV_ID);
  at25_receive_byte(&mfid);
  at25_receive_byte(&devid);
  at25_unselect();
  return mfid == 0x1F && devid == 0x47;
}

void at25_global_unprotect()
{
  at25_write_enable();
  at25_select();
  at25_transmit_byte(AT25_CMD_WRITE_STATUS_1);
  at25_transmit_byte(0x0); // SPRL and 5,4,3,2 bits unset
  at25_unselect();
}

void at25_erase_all()
{
  at25_write_enable();
  at25_select();
  at25_transmit_byte(AT25_CMD_ERASE_ALL);
  at25_unselect();
}

uint8_t at25_write_block(uint32_t address, uint8_t *data, uint16_t count, uint32_t timeout)
{
  uint32_t tickstart = HAL_GetTick();
  while (!at25_is_ready()) {
    if (HAL_GetTick() - tickstart > timeout) {
      return 0;
    }
  }
  at25_write_enable();
  at25_select();
  at25_transmit_byte(AT25_CMD_BYTE_PROGRAM);
  at25_transmit_byte((address >> 16) & 0x3F);
  at25_transmit_byte((address >> 8)  & 0xFF);
  at25_transmit_byte( address        & 0xFF);
  for (uint16_t i=0; i < count; ++i) {
    at25_transmit_byte(data[i]);
  }
  at25_unselect();

  while (!at25_is_ready()) {
    if (HAL_GetTick() - tickstart > timeout) {
      return 0;
    }
  }
  return at25_write_ok();
}

void at25_read_block(uint32_t address, uint8_t *data, uint16_t count)
{
  at25_select();
  at25_transmit_byte(AT25_CMD_READ_ARRAY);
  at25_transmit_byte((address >> 16) & 0x3F);
  at25_transmit_byte((address >> 8)  & 0xFF);
  at25_transmit_byte( address        & 0xFF);

  for (uint16_t i=0; i < count; ++i) {
    at25_receive_byte(&data[i]);
  }
  at25_unselect();
}
